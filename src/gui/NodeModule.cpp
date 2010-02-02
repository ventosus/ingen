/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
 *
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <cassert>
#include "ingen-config.h"
#include "raul/log.hpp"
#include "raul/Atom.hpp"
#include "interface/EngineInterface.hpp"
#include "shared/LV2URIMap.hpp"
#include "client/PatchModel.hpp"
#include "client/NodeModel.hpp"
#include "client/PluginModel.hpp"
#include "client/PluginUI.hpp"
#include "App.hpp"
#include "GladeFactory.hpp"
#include "NodeControlWindow.hpp"
#include "NodeModule.hpp"
#include "PatchCanvas.hpp"
#include "PatchWindow.hpp"
#include "Port.hpp"
#include "RenameWindow.hpp"
#include "SubpatchModule.hpp"
#include "WindowFactory.hpp"
#include "Configuration.hpp"
#include "NodeMenu.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace GUI {


NodeModule::NodeModule(boost::shared_ptr<PatchCanvas> canvas, SharedPtr<NodeModel> node)
	: FlowCanvas::Module(canvas, node->path().symbol(), 0, 0, true, canvas->show_port_names())
	, _node(node)
	, _gui_widget(NULL)
	, _gui_window(NULL)
{
	assert(_node);

	node->signal_new_port.connect(sigc::bind(sigc::mem_fun(this, &NodeModule::add_port), true));
	node->signal_removed_port.connect(sigc::hide_return(sigc::mem_fun(this, &NodeModule::remove_port)));
	node->signal_property.connect(sigc::mem_fun(this, &NodeModule::set_property));
	node->signal_moved.connect(sigc::mem_fun(this, &NodeModule::rename));
	PluginModel* plugin = dynamic_cast<PluginModel*>(node->plugin());
	if (plugin)
		plugin->signal_changed.connect(sigc::mem_fun(this, &NodeModule::plugin_changed));
}


NodeModule::~NodeModule()
{
	NodeControlWindow* win = App::instance().window_factory()->control_window(_node);

	if (win) {
		// Should remove from window factory via signal
		delete win;
	}
}


void
NodeModule::create_menu()
{
	Glib::RefPtr<Gnome::Glade::Xml> xml = GladeFactory::new_glade_reference();
	xml->get_widget_derived("object_menu", _menu);
	_menu->init(_node);
	_menu->signal_embed_gui.connect(sigc::mem_fun(this, &NodeModule::embed_gui));
	_menu->signal_popup_gui.connect(sigc::hide_return(sigc::mem_fun(this, &NodeModule::popup_gui)));

	set_menu(_menu);
}


boost::shared_ptr<NodeModule>
NodeModule::create(boost::shared_ptr<PatchCanvas> canvas, SharedPtr<NodeModel> node, bool human)
{
	boost::shared_ptr<NodeModule> ret;

	SharedPtr<PatchModel> patch = PtrCast<PatchModel>(node);
	if (patch)
		ret = boost::shared_ptr<NodeModule>(new SubpatchModule(canvas, patch));
	else
		ret = boost::shared_ptr<NodeModule>(new NodeModule(canvas, node));

	for (GraphObject::Properties::const_iterator m = node->meta().properties().begin(); m != node->meta().properties().end(); ++m)
		ret->set_property(m->first, m->second);

	for (GraphObject::Properties::const_iterator m = node->properties().begin(); m != node->properties().end(); ++m)
		ret->set_property(m->first, m->second);

	for (NodeModel::Ports::const_iterator p = node->ports().begin(); p != node->ports().end(); ++p)
		ret->add_port(*p, false);

	ret->set_stacked_border(node->polyphonic());

	if (human)
		ret->show_human_names(human); // FIXME: double port iteration
	else
		ret->resize();

	return ret;
}


void
NodeModule::show_human_names(bool b)
{
	if (b && node()->plugin()) {
		Glib::Mutex::Lock lock(App::instance().world()->rdf_world->mutex());
		set_name(((PluginModel*)node()->plugin())->human_name());
	} else {
		b = false;
	}

	if (!b)
		set_name(node()->symbol().c_str());

	uint32_t index = 0;
	for (PortVector::const_iterator i = ports().begin(); i != ports().end(); ++i) {
		Glib::ustring label(node()->port(index)->symbol().c_str());
		if (b) {
			Glib::ustring hn = ((PluginModel*)node()->plugin())->port_human_name(index);
			if (hn != "")
				label = hn;
		}
		(*i)->set_name(label);
		++index;
	}

	resize();
}


void
NodeModule::value_changed(uint32_t index, const Atom& value)
{
	if (!_plugin_ui)
		return;

	float                   float_val = 0.0f;
	SLV2UIInstance          inst      = _plugin_ui->instance();
	const LV2UI_Descriptor* ui_desc   = slv2_ui_instance_get_descriptor(inst);
	LV2UI_Handle            ui        = slv2_ui_instance_get_handle(inst);

	switch (value.type()) {
	case Atom::FLOAT:
		float_val = value.get_float();
		if (ui_desc->port_event)
			ui_desc->port_event(ui, index, 4, 0, &float_val);
		break;
	case Atom::STRING:
		if (ui_desc->port_event)
			ui_desc->port_event(ui, index, strlen(value.get_string()), 0, value.get_string());
		break;
	default:
		break;
	}
}


void
NodeModule::plugin_changed()
{
	for (PortVector::iterator p = ports().begin(); p != ports().end(); ++p)
		PtrCast<Ingen::GUI::Port>(*p)->update_metadata();
}


void
NodeModule::embed_gui(bool embed)
{
	if (embed) {

		if (_gui_window) {
			warn << "LV2 GUI already popped up, cannot embed" << endl;
			return;
		}

		if (!_plugin_ui) {
			const PluginModel* const pm = dynamic_cast<const PluginModel*>(_node->plugin());
			assert(pm);
			_plugin_ui = pm->ui(App::instance().world(), _node);
		}

		if (_plugin_ui) {
			GtkWidget* c_widget = (GtkWidget*)slv2_ui_instance_get_widget(_plugin_ui->instance());
			_gui_widget = Glib::wrap(c_widget);
			assert(_gui_widget);

			Gtk::Container* container = new Gtk::EventBox();
			container->set_name("ingen_embedded_node_gui_container");
			container->add(*_gui_widget);
			FlowCanvas::Module::embed(container);
		} else {
			error << "Failed to create LV2 UI" << endl;
		}

		if (_gui_widget) {
			_gui_widget->show_all();

			for (NodeModel::Ports::const_iterator p = _node->ports().begin();
					p != _node->ports().end(); ++p)
				if ((*p)->type().is_control() && (*p)->is_output())
					App::instance().engine()->set_property((*p)->path(), "ingen:broadcast", true);
		}

	} else { // un-embed

		FlowCanvas::Module::embed(NULL);
		_plugin_ui.reset();

		for (NodeModel::Ports::const_iterator p = _node->ports().begin(); p != _node->ports().end(); ++p)
			if ((*p)->type().is_control() && (*p)->is_output())
				App::instance().engine()->set_property((*p)->path(), "ingen:broadcast", false);
	}

	if (embed && _embed_item) {
		set_control_values();
		set_base_color(0x212222FF);
	} else {
		set_default_base_color();
	}

	resize();
}


void
NodeModule::rename()
{
	if (App::instance().configuration()->name_style() == Configuration::PATH) {
		set_name(_node->path().symbol());
		resize();
	}
}


void
NodeModule::add_port(SharedPtr<PortModel> port, bool resize_to_fit)
{
	Module::add_port(Port::create(PtrCast<NodeModule>(shared_from_this()), port,
			App::instance().configuration()->name_style() == Configuration::HUMAN));

	port->signal_value_changed.connect(sigc::bind<0>(
			sigc::mem_fun(this, &NodeModule::value_changed), port->index()));

	if (resize_to_fit)
		resize();
}


boost::shared_ptr<Port>
NodeModule::port(boost::shared_ptr<PortModel> model)
{
	for (PortVector::const_iterator p = ports().begin(); p != ports().end(); ++p) {
		SharedPtr<Port> port = PtrCast<Port>(*p);
		if (port->model() == model)
			return port;
	}
	return boost::shared_ptr<Port>();
}


void
NodeModule::remove_port(SharedPtr<PortModel> model)
{
	SharedPtr<Port> p = port(model);
	if (p) {
		Module::remove_port(p);
		p.reset();
	} else {
		warn << "Failed to find port on module " << model->path() << endl;
	}
}


bool
NodeModule::popup_gui()
{
#ifdef HAVE_SLV2
	if (_node->plugin() && _node->plugin()->type() == PluginModel::LV2) {
		if (_plugin_ui) {
			warn << "LV2 GUI already embedded, cannot pop up" << endl;
			return false;
		}

		const PluginModel* const plugin = dynamic_cast<const PluginModel*>(_node->plugin());
		assert(plugin);

		_plugin_ui = plugin->ui(App::instance().world(), _node);

		if (_plugin_ui) {
			GtkWidget* c_widget = (GtkWidget*)slv2_ui_instance_get_widget(_plugin_ui->instance());
			_gui_widget = Glib::wrap(c_widget);

			_gui_window = new Gtk::Window();
			_gui_window->set_title(_node->path().chop_scheme() + " UI - Ingen");
			_gui_window->set_role("plugin_ui");
			_gui_window->add(*_gui_widget);
			_gui_widget->show_all();
			set_control_values();

			_gui_window->signal_unmap().connect(
					sigc::mem_fun(this, &NodeModule::on_gui_window_close));
			_gui_window->present();

			return true;
		} else {
			warn << "No LV2 GUI for " << _node->path() << endl;
		}
	}
#endif
	return false;
}


void
NodeModule::on_gui_window_close()
{
	delete _gui_window;
	_gui_window = NULL;
	_plugin_ui.reset();
	_gui_widget = NULL;
}


void
NodeModule::set_control_values()
{
	uint32_t index=0;
	for (NodeModel::Ports::const_iterator p = _node->ports().begin(); p != _node->ports().end(); ++p) {
		if ((*p)->type().is_control())
			value_changed(index, (*p)->value());
		++index;
	}
}


void
NodeModule::show_control_window()
{
	App::instance().window_factory()->present_controls(_node);
}


void
NodeModule::on_double_click(GdkEventButton* ev)
{
	if ( ! popup_gui() )
		show_control_window();
}


void
NodeModule::store_location()
{
	const float x = static_cast<float>(property_x());
	const float y = static_cast<float>(property_y());

	const LV2URIMap& uris = App::instance().uris();

	const Atom& existing_x = _node->get_property(uris.ingenui_canvas_x);
	const Atom& existing_y = _node->get_property(uris.ingenui_canvas_y);

	if (existing_x.type() != Atom::FLOAT || existing_y.type() != Atom::FLOAT
			|| existing_x.get_float() != x || existing_y.get_float() != y) {
		Shared::Resource::Properties props;
		props.insert(make_pair(uris.ingenui_canvas_x, Atom(x)));
		props.insert(make_pair(uris.ingenui_canvas_y, Atom(y)));
		App::instance().engine()->put(_node->path(), props);
	}
}


void
NodeModule::set_property(const URI& key, const Atom& value)
{
	const Shared::LV2URIMap& uris = App::instance().uris();
	switch (value.type()) {
	case Atom::FLOAT:
		if (key == uris.ingenui_canvas_x) {
			move_to(value.get_float(), property_y());
		} else if (key == uris.ingenui_canvas_y) {
			move_to(property_x(), value.get_float());
		}
		break;
	case Atom::BOOL:
		if (key == uris.ingen_polyphonic) {
			set_stacked_border(value.get_bool());
		} else if (key == uris.ingen_selected) {
			if (value.get_bool() != selected()) {
				if (value.get_bool())
					_canvas.lock()->select_item(shared_from_this());
				else
					_canvas.lock()->unselect_item(shared_from_this());
			}
		}
	default: break;
	}
}


void
NodeModule::set_selected(bool b)
{
	const LV2URIMap& uris = App::instance().uris();
	if (b != selected()) {
		Module::set_selected(b);
		if (App::instance().signal())
			App::instance().engine()->set_property(_node->path(), uris.ingen_selected, b);
	}
}


} // namespace GUI
} // namespace Ingen
