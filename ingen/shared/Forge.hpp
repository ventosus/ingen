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

#ifndef INGEN_FORGE_HPP
#define INGEN_FORGE_HPP

#include "ingen/shared/Forge.hpp"
#include "ingen/shared/LV2URIMap.hpp"
#include "raul/Atom.hpp"

namespace Ingen {

class Forge : public Raul::Forge {
public:
	Forge(Shared::LV2URIMap& map);

	std::string str(const Raul::Atom& atom);
};

} // namespace Ingen

#endif // INGEN_FORGE_HPP