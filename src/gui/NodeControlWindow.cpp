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

#include <cmath>
#include "ingen/Interface.hpp"
#include "ingen/shared/LV2URIMap.hpp"
#include "ingen/client/NodeModel.hpp"
#include "App.hpp"
#include "NodeControlWindow.hpp"
#include "WidgetFactory.hpp"
#include "Controls.hpp"
#include "ControlPanel.hpp"
#include "PatchWindow.hpp"

using namespace std;

namespace Ingen {

using namespace Client;

namespace GUI {

/** Create a node control window and load a new ControlPanel for it.
 */
NodeControlWindow::NodeControlWindow(App&                               app,
                                     SharedPtr<const Client::NodeModel> node,
                                     uint32_t                           poly)
	: _node(node)
	, _position_stored(false)
	, _x(0)
	, _y(0)
{
	assert(_node);

	property_resizable() = true;
	set_border_width(5);

	set_title(_node->plugin_model()->human_name() + " - Ingen");

	Glib::RefPtr<Gtk::Builder> xml = WidgetFactory::create("warehouse_win");
	xml->get_widget_derived("control_panel_vbox", _control_panel);

	show_all_children();

	_control_panel->reparent(*this);
	_control_panel->init(app, _node, poly);
	_control_panel->show();

	resize();

	_callback_enabled = true;
}

/** Create a node control window and with an existing ControlPanel.
 */
NodeControlWindow::NodeControlWindow(App&                       app,
                                     SharedPtr<const NodeModel> node,
                                     ControlPanel*              panel)
	: _node(node)
	, _control_panel(panel)
{
	assert(_node);

	property_resizable() = true;
	set_border_width(5);

	set_title(_node->path().chop_scheme() + " Controls - Ingen");

	_control_panel->reparent(*this);

	show_all_children();
	resize();

	_callback_enabled = true;
}

NodeControlWindow::~NodeControlWindow()
{
	delete _control_panel;
}

void
NodeControlWindow::resize()
{
	pair<int,int> controls_size = _control_panel->ideal_size();
	int width  = controls_size.first;
	int height = controls_size.second;

	if (height > property_screen().get_value()->get_height() - 64)
		height = property_screen().get_value()->get_height() - 64;
	if (width > property_screen().get_value()->get_width() - 64)
		width = property_screen().get_value()->get_width() - 64;

	Gtk::Window::resize(width, height);
}

void
NodeControlWindow::on_show()
{
	if (_position_stored)
		move(_x, _y);

	Gtk::Window::on_show();
}

void
NodeControlWindow::on_hide()
{
	_position_stored = true;
	get_position(_x, _y);
	Gtk::Window::on_hide();
}

} // namespace GUI
} // namespace Ingen
