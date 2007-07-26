/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#ifndef NODEPROPERTIESWINDOW_H
#define NODEPROPERTIESWINDOW_H

#include <gtkmm.h>
#include <libglademm.h>
#include <raul/SharedPtr.hpp>
#include "client/NodeModel.hpp"
using namespace Ingen::Client;

namespace Ingen {
namespace GUI {


/** Node properties window.
 *
 * Loaded by libglade as a derived object.
 *
 * \ingroup GUI
 */
class NodePropertiesWindow : public Gtk::Window
{
public:
	NodePropertiesWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& refGlade);

	void present(SharedPtr<NodeModel> node_model) { set_node(node_model); Gtk::Window::present(); }
	void set_node(SharedPtr<NodeModel> node_model);

private:
	
	SharedPtr<NodeModel> _node_model;
	Gtk::Label*           _node_path_label;
	Gtk::CheckButton*     _node_polyphonic_toggle;
	Gtk::Label*           _plugin_type_label;
	Gtk::Label*           _plugin_uri_label;
	Gtk::Label*           _plugin_name_label;
};

} // namespace GUI
} // namespace Ingen

#endif // NODEPROPERTIESWINDOW_H