/* This file is part of Ingen.
 * Copyright 2012 David Robillard <http://drobilla.net>
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

#ifndef INGEN_SHARED_ATOM_READER_HPP
#define INGEN_SHARED_ATOM_READER_HPP

#include "ingen/Interface.hpp"
#include "ingen/shared/AtomSink.hpp"
#include "ingen/shared/LV2URIMap.hpp"
#include "ingen/shared/URIs.hpp"
#include "serd/serd.h"

namespace Ingen {

class Forge;

namespace Shared {

class AtomSink;

/** An AtomSink that calls methods on an Interface. */
class AtomReader : public AtomSink
{
public:
	AtomReader(LV2URIMap& map, URIs& uris, Forge& forge, Interface& iface);
	~AtomReader() {}

	void write(const LV2_Atom* msg);

private:
	LV2URIMap& _map;
	URIs&      _uris;
	Forge&     _forge;
	Interface& _iface;
};

} // namespace Shared
} // namespace Ingen

#endif // INGEN_SHARED_ATOM_READER_HPP
