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

#include "GladeFactory.h"
#include <iostream>
#include <fstream>
using std::cout; using std::cerr; using std::endl;
using std::ifstream;

namespace Ingen {
namespace GUI {


Glib::ustring GladeFactory::glade_filename = "";


void
GladeFactory::find_glade_file()
{
	// Check for the .glade file in current directory
	glade_filename = "./ingenuity.glade";
	ifstream fs(glade_filename.c_str());
	if (fs.fail()) { // didn't find it, check PKGDATADIR
		fs.clear();
		glade_filename = PKGDATADIR;
		glade_filename += "/ingenuity.glade";
	
		fs.open(glade_filename.c_str());
		if (fs.fail()) {
			cerr << "[GladeFactory] Unable to find ingenuity.glade in current directory or " << PKGDATADIR << "." << endl;
			throw;
		}
		fs.close();
	}
	cerr << "[GladeFactory] Loading widgets from " << glade_filename.c_str() << endl;
}


Glib::RefPtr<Gnome::Glade::Xml>
GladeFactory::new_glade_reference(const string& toplevel_widget)
{
	if (glade_filename == "")
		find_glade_file();

	try {
		if (toplevel_widget == "")
			return Gnome::Glade::Xml::create(glade_filename);
		else
			return Gnome::Glade::Xml::create(glade_filename, toplevel_widget.c_str());
	} catch (const Gnome::Glade::XmlError& ex) {
		cerr << ex.what() << endl;
		throw ex;
	}
}


} // namespace GUI
} // namespace Ingen