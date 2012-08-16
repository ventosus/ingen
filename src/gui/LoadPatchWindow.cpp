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
#include <list>
#include <string>

#include <boost/optional.hpp>
#include <glibmm/miscutils.h>

#include "ingen/Interface.hpp"
#include "ingen/client/ClientStore.hpp"
#include "ingen/client/NodeModel.hpp"
#include "ingen/client/PatchModel.hpp"
#include "ingen/runtime_paths.hpp"

#include "App.hpp"
#include "Configuration.hpp"
#include "LoadPatchWindow.hpp"
#include "PatchView.hpp"
#include "ThreadedLoader.hpp"

using namespace std;

namespace Ingen {

using namespace Client;

namespace GUI {

LoadPatchWindow::LoadPatchWindow(BaseObjectType*                   cobject,
                                 const Glib::RefPtr<Gtk::Builder>& xml)
	: Gtk::FileChooserDialog(cobject)
	, _app(NULL)
	, _merge_ports(false)
{
	xml->get_widget("load_patch_symbol_label", _symbol_label);
	xml->get_widget("load_patch_symbol_entry", _symbol_entry);
	xml->get_widget("load_patch_ports_label", _ports_label);
	xml->get_widget("load_patch_merge_ports_radio", _merge_ports_radio);
	xml->get_widget("load_patch_insert_ports_radio", _insert_ports_radio);
	xml->get_widget("load_patch_poly_voices_radio", _poly_voices_radio);
	xml->get_widget("load_patch_poly_from_file_radio", _poly_from_file_radio);
	xml->get_widget("load_patch_poly_spinbutton", _poly_spinbutton);
	xml->get_widget("load_patch_ok_button", _ok_button);
	xml->get_widget("load_patch_cancel_button", _cancel_button);

	_cancel_button->signal_clicked().connect(
			sigc::mem_fun(this, &LoadPatchWindow::cancel_clicked));
	_ok_button->signal_clicked().connect(
			sigc::mem_fun(this, &LoadPatchWindow::ok_clicked));
	_merge_ports_radio->signal_toggled().connect(
			sigc::mem_fun(this, &LoadPatchWindow::merge_ports_selected));
	_insert_ports_radio->signal_toggled().connect(
			sigc::mem_fun(this, &LoadPatchWindow::insert_ports_selected));
	_poly_from_file_radio->signal_toggled().connect(sigc::bind(
			sigc::mem_fun(_poly_spinbutton, &Gtk::SpinButton::set_sensitive),
			false));
	_poly_voices_radio->signal_toggled().connect(sigc::bind(
			sigc::mem_fun(_poly_spinbutton, &Gtk::SpinButton::set_sensitive),
			true));

	signal_selection_changed().connect(
			sigc::mem_fun(this, &LoadPatchWindow::selection_changed));

	Gtk::FileFilter filt;
	filt.add_pattern("*.ttl");
	filt.set_name("Ingen patch files (*.ttl)");
	filt.add_pattern("*.ingen");
	filt.set_name("Ingen bundles (*.ingen)");

	set_filter(filt);

	property_select_multiple() = true;

	// Add global examples directory to "shortcut folders" (bookmarks)
	const string examples_dir = Ingen::data_file_path("patches");
	if (Glib::file_test(examples_dir, Glib::FILE_TEST_IS_DIR)) {
		add_shortcut_folder(examples_dir);
	}
}

void
LoadPatchWindow::present(SharedPtr<const PatchModel> patch,
                         bool                        import,
                         GraphObject::Properties     data)
{
	_import = import;
	set_patch(patch);
	_symbol_label->property_visible() = !import;
	_symbol_entry->property_visible() = !import;
	_ports_label->property_visible() = _import;
	_merge_ports_radio->property_visible() = _import;
	_insert_ports_radio->property_visible() = _import;
	_initial_data = data;
	Gtk::Window::present();
}

/** Sets the patch model for this window and initializes everything.
 *
 * This function MUST be called before using the window in any way!
 */
void
LoadPatchWindow::set_patch(SharedPtr<const PatchModel> patch)
{
	_patch = patch;
	_symbol_entry->set_text("");
	_symbol_entry->set_sensitive(!_import);
	_poly_spinbutton->set_value(patch->internal_poly());
}

void
LoadPatchWindow::on_show()
{
	if (_app->configuration()->patch_folder().length() > 0)
		set_current_folder(_app->configuration()->patch_folder());
	Gtk::FileChooserDialog::on_show();
}

void
LoadPatchWindow::merge_ports_selected()
{
	_merge_ports = true;
}

void
LoadPatchWindow::insert_ports_selected()
{
	_merge_ports = false;
}

void
LoadPatchWindow::ok_clicked()
{
	if (!_patch) {
		hide();
		return;
	}

	const URIs& uris = _app->uris();

	if (_poly_voices_radio->get_active())
		_initial_data.insert(
			make_pair(uris.ingen_polyphony,
			          _app->forge().make(_poly_spinbutton->get_value_as_int())));

	if (get_uri() == "")
		return;

	if (_import) {
		// If unset load_patch will load value
		boost::optional<Raul::Path>   parent;
		boost::optional<Raul::Symbol> symbol;
		if (!_patch->path().is_root()) {
			parent = _patch->path().parent();
			symbol = _patch->symbol();
		}

		_app->loader()->load_patch(true, get_filename(),
				parent, symbol, _initial_data);

	} else {
		std::list<Glib::ustring> uri_list = get_filenames();
		for (std::list<Glib::ustring>::iterator i = uri_list.begin(); i != uri_list.end(); ++i) {
			// Cascade
			Raul::Atom& x = _initial_data.find(uris.ingen_canvasX)->second;
			x = _app->forge().make(x.get_float() + 20.0f);
			Raul::Atom& y = _initial_data.find(uris.ingen_canvasY)->second;
			y = _app->forge().make(y.get_float() + 20.0f);

			Raul::Symbol symbol(symbol_from_filename(*i));
			if (uri_list.size() == 1 && _symbol_entry->get_text() != "")
				symbol = Raul::Symbol::symbolify(_symbol_entry->get_text());

			symbol = avoid_symbol_clash(symbol);

			_app->loader()->load_patch(false, *i,
					_patch->path(), symbol, _initial_data);
		}
	}

	_patch.reset();
	hide();
}

void
LoadPatchWindow::cancel_clicked()
{
	_patch.reset();
	hide();
}

Raul::Symbol
LoadPatchWindow::symbol_from_filename(const Glib::ustring& filename)
{
	std::string symbol_str = Glib::path_get_basename(get_filename());
	symbol_str = symbol_str.substr(0, symbol_str.find('.'));
	return Raul::Symbol::symbolify(symbol_str);
}

Raul::Symbol
LoadPatchWindow::avoid_symbol_clash(const Raul::Symbol& symbol)
{
	unsigned offset = _app->store()->child_name_offset(
			_patch->path(), symbol);

	if (offset != 0) {
		std::stringstream ss;
		ss << symbol << "_" << offset;
		return Raul::Symbol(ss.str());
	} else {
		return symbol;
	}
}

void
LoadPatchWindow::selection_changed()
{
	if (_import)
		return;

	if (get_filenames().size() != 1) {
		_symbol_entry->set_text("");
		_symbol_entry->set_sensitive(false);
	} else {
		_symbol_entry->set_text(avoid_symbol_clash(
				symbol_from_filename(get_filename())).c_str());
		_symbol_entry->set_sensitive(true);
	}
}

} // namespace GUI
} // namespace Ingen
