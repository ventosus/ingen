/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
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
#include <string>
#include <glibmm.h>

#include "raul/log.hpp"

#include "sord/sordmm.hpp"

#include "shared/LV2URIMap.hpp"
#include "Driver.hpp"
#include "Engine.hpp"
#include "LV2Node.hpp"
#include "LV2Plugin.hpp"
#include "NodeImpl.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Server {

LV2Plugin::LV2Plugin(SharedPtr<LV2Info> lv2_info, const std::string& uri)
	: PluginImpl(*lv2_info->world().uris().get(), Plugin::LV2, uri)
	, _lilv_plugin(NULL)
	, _lv2_info(lv2_info)
{
	set_property(_uris.rdf_type, _uris.lv2_Plugin);
}

const string
LV2Plugin::symbol() const
{
	string working = uri().str();
	if (working[working.length()-1] == '/')
		working = working.substr(0, working.length()-1);

	while (working.length() > 0) {
		size_t last_slash = working.find_last_of("/");
		const string symbol = working.substr(last_slash+1);
		if ( (symbol[0] >= 'a' && symbol[0] <= 'z')
				|| (symbol[0] >= 'A' && symbol[0] <= 'Z') )
			return Path::nameify(symbol);
		else
			working = working.substr(0, last_slash);
	}

	return "lv2_symbol";
}

NodeImpl*
LV2Plugin::instantiate(BufferFactory& bufs,
                       const string&  name,
                       bool           polyphonic,
                       PatchImpl*     parent,
                       Engine&        engine)
{
	const SampleCount srate = engine.driver()->sample_rate();

	load(); // FIXME: unload at some point

	LV2Node* n = new LV2Node(this, name, polyphonic, parent, srate);

	if ( ! n->instantiate(bufs) ) {
		delete n;
		n = NULL;
	}

	return n;
}

void
LV2Plugin::lilv_plugin(LilvPlugin p)
{
	_lilv_plugin = p;
}

const std::string&
LV2Plugin::library_path() const
{
	static const std::string empty_string;
	if (_library_path.empty()) {
		LilvValue v = lilv_plugin_get_library_uri(_lilv_plugin);
		if (v) {
			_library_path = lilv_uri_to_path(lilv_value_as_uri(v));
		} else {
			Raul::warn << uri() << " has no library path" << std::endl;
			return empty_string;
		}
	}

	return _library_path;
}

} // namespace Server
} // namespace Ingen