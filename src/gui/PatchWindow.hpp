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

#ifndef PATCHWINDOW_H
#define PATCHWINDOW_H

#include <string>
#include <list>
#include <gtkmm.h>
#include <libglademm/xml.h>
#include <libglademm.h>
#include <raul/Path.hpp>
#include <raul/SharedPtr.hpp>
#include "client/PatchModel.hpp"
#include "PatchView.hpp"
using Ingen::Client::PatchModel;

using std::string; using std::list;


namespace Ingen { namespace Client {
	class PatchModel;
	class PortModel;
	class MetadataModel;
} }
using namespace Ingen::Client;


namespace Ingen {
namespace GUI {
	
class LoadPluginWindow;
class LoadPatchWindow;
class NewSubpatchWindow;
class LoadSubpatchWindow;
class NewSubpatchWindow;
class NodeControlWindow;
class PatchDescriptionWindow;
class SubpatchModule;
class OmPort;
class BreadCrumbBox;


/** A window for a patch.
 *
 * \ingroup GUI
 */
class PatchWindow : public Gtk::Window
{
public:
	PatchWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade_xml);
	~PatchWindow();
	
	void set_patch_from_path(const Path& path, SharedPtr<PatchView> view);
	void set_patch(SharedPtr<PatchModel> pc, SharedPtr<PatchView> view);

	SharedPtr<PatchModel> patch() const { return _patch; }

	Gtk::MenuItem* menu_view_control_window() { return _menu_view_control_window; }

protected:
	void on_show();
	void on_hide();
	bool on_key_press_event(GdkEventKey* event);
	bool on_key_release_event(GdkEventKey* event);
	
private:

	void patch_port_added(SharedPtr<PortModel> port);
	void patch_port_removed(SharedPtr<PortModel> port);

	void event_import();
	void event_import_location();
	void event_save();
	void event_save_as();
	void event_upload();
	void event_copy();
	void event_paste();
	void event_delete();
	void event_select_all();
	void event_quit();
	void event_destroy();
	void event_clear();
	void event_fullscreen_toggled();
	void event_human_names_toggled();
	void event_arrange();
	void event_show_properties();
	void event_show_controls();
	void event_show_engine();
	void event_clipboard_changed(GdkEventOwnerChange* ev);

	SharedPtr<PatchModel> _patch;
	SharedPtr<PatchView>  _view;
	
	sigc::connection new_port_connection;
	sigc::connection removed_port_connection;

	bool _enable_signal;
	bool _position_stored;
	int  _x;
	int  _y;
	
	Gtk::MenuItem*      _menu_import;
	Gtk::MenuItem*      _menu_import_location;
	Gtk::MenuItem*      _menu_save;
	Gtk::MenuItem*      _menu_save_as;
	Gtk::MenuItem*      _menu_upload;
	Gtk::MenuItem*      _menu_cut;
	Gtk::MenuItem*      _menu_copy;
	Gtk::MenuItem*      _menu_paste;
	Gtk::MenuItem*      _menu_delete;
	Gtk::MenuItem*      _menu_select_all;
	Gtk::MenuItem*      _menu_close;
	Gtk::MenuItem*      _menu_quit;
	Gtk::CheckMenuItem* _menu_human_names;
	Gtk::MenuItem*      _menu_fullscreen;
	Gtk::MenuItem*      _menu_clear;
	Gtk::MenuItem*      _menu_destroy_patch;
	Gtk::MenuItem*      _menu_arrange;
	Gtk::MenuItem*      _menu_view_engine_window;
	Gtk::MenuItem*      _menu_view_control_window;
	Gtk::MenuItem*      _menu_view_patch_properties;
	Gtk::MenuItem*      _menu_view_messages_window;
	Gtk::MenuItem*      _menu_view_patch_tree_window;
	Gtk::MenuItem*      _menu_help_about;
	
	Gtk::VBox*          _vbox;
	Gtk::Viewport*      _viewport;
	BreadCrumbBox*      _breadcrumb_box;
	
	//Gtk::Statusbar*   _status_bar;
	
	/** Invisible bin used to store breadcrumbs when not shown by a view */
	Gtk::Alignment _breadcrumb_bin;
};


} // namespace GUI
} // namespace Ingen

#endif // PATCHWINDOW_H