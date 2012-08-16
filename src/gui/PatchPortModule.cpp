/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cassert>
#include <string>
#include <utility>

#include "ingen/Interface.hpp"
#include "ingen/client/NodeModel.hpp"
#include "ingen/client/PatchModel.hpp"

#include "App.hpp"
#include "Configuration.hpp"
#include "PatchCanvas.hpp"
#include "PatchPortModule.hpp"
#include "PatchWindow.hpp"
#include "Port.hpp"
#include "PortMenu.hpp"
#include "RenameWindow.hpp"
#include "WidgetFactory.hpp"
#include "WindowFactory.hpp"

using namespace std;

namespace Ingen {

using namespace Client;

namespace GUI {

PatchPortModule::PatchPortModule(PatchCanvas&                       canvas,
                                 SharedPtr<const Client::PortModel> model)
	: Ganv::Module(canvas, "", 0, 0, false) // FIXME: coords?
	, _model(model)
{
	assert(model);

	assert(PtrCast<const PatchModel>(model->parent()));

	set_stacked(model->polyphonic());

	model->signal_property().connect(
		sigc::mem_fun(this, &PatchPortModule::property_changed));

	signal_moved().connect(
		sigc::mem_fun(this, &PatchPortModule::store_location));
}

PatchPortModule*
PatchPortModule::create(PatchCanvas&               canvas,
                        SharedPtr<const PortModel> model,
                        bool                       human)
{
	PatchPortModule* ret  = new PatchPortModule(canvas, model);
	Port*            port = Port::create(canvas.app(), *ret, model, human, true);

	ret->set_port(port);

	for (GraphObject::Properties::const_iterator m = model->properties().begin();
	     m != model->properties().end(); ++m)
		ret->property_changed(m->first, m->second);

	return ret;
}

App&
PatchPortModule::app() const
{
	return ((PatchCanvas*)canvas())->app();
}

bool
PatchPortModule::show_menu(GdkEventButton* ev)
{
	return _port->show_menu(ev);
}

void
PatchPortModule::store_location(double ax, double ay)
{
	const URIs& uris = app().uris();

	const Raul::Atom x(app().forge().make(static_cast<float>(ax)));
	const Raul::Atom y(app().forge().make(static_cast<float>(ay)));

	if (x != _model->get_property(uris.ingen_canvasX) ||
	    y != _model->get_property(uris.ingen_canvasY))
	{
		Resource::Properties remove;
		remove.insert(make_pair(uris.ingen_canvasX, uris.wildcard));
		remove.insert(make_pair(uris.ingen_canvasY, uris.wildcard));
		Resource::Properties add;
		add.insert(make_pair(uris.ingen_canvasX,
		                     Resource::Property(x, Resource::INTERNAL)));
		add.insert(make_pair(uris.ingen_canvasY,
		                     Resource::Property(y, Resource::INTERNAL)));
		app().interface()->delta(_model->uri(), remove, add);
	}
}

void
PatchPortModule::show_human_names(bool b)
{
	const URIs&       uris = app().uris();
	const Raul::Atom& name = _model->get_property(uris.lv2_name);
	if (b && name.type() == uris.forge.String) {
		set_name(name.get_string());
	} else {
		set_name(_model->symbol().c_str());
	}
}

void
PatchPortModule::set_name(const std::string& n)
{
	_port->set_label(n.c_str());
}

void
PatchPortModule::property_changed(const Raul::URI& key, const Raul::Atom& value)
{
	const URIs& uris = app().uris();
	if (value.type() == uris.forge.Float) {
		if (key == uris.ingen_canvasX) {
			move_to(value.get_float(), get_y());
		} else if (key == uris.ingen_canvasY) {
			move_to(get_x(), value.get_float());
		}
	} else if (value.type() == uris.forge.String) {
		if (key == uris.lv2_name
		    && app().configuration()->name_style() == Configuration::HUMAN) {
			set_name(value.get_string());
		} else if (key == uris.lv2_symbol
		           && app().configuration()->name_style() == Configuration::PATH) {
			set_name(value.get_string());
		}
	} else if (value.type() == uris.forge.Bool) {
		if (key == uris.ingen_polyphonic) {
			set_stacked(value.get_bool());
		}
	}
}

void
PatchPortModule::set_selected(gboolean b)
{
	if (b != get_selected()) {
		Module::set_selected(b);
	}
}

} // namespace GUI
} // namespace Ingen
