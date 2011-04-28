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

#include <string>

#include "GraphObjectImpl.hpp"
#include "PatchImpl.hpp"
#include "EngineStore.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Server {

GraphObjectImpl::GraphObjectImpl(Ingen::Shared::LV2URIMap& uris,
                                 GraphObjectImpl*          parent,
                                 const Symbol&             symbol)
	: ResourceImpl(uris, parent ? parent->path().child(symbol) : Raul::Path::root())
	, _parent(parent)
	, _path(parent ? parent->path().child(symbol) : "/")
	, _symbol(symbol)
{
}

void
GraphObjectImpl::add_meta_property(const Raul::URI& key, const Atom& value)
{
	add_property(key, Resource::Property(value, Resource::INTERNAL));
}

void
GraphObjectImpl::set_meta_property(const Raul::URI& key, const Atom& value)
{
	set_property(key, Resource::Property(value, Resource::INTERNAL));
}

const Atom&
GraphObjectImpl::get_property(const Raul::URI& key) const
{
	static const Atom null_atom;
	Resource::Properties::const_iterator i = properties().find(key);
	return (i != properties().end()) ? i->second : null_atom;
}

PatchImpl*
GraphObjectImpl::parent_patch() const
{
	return dynamic_cast<PatchImpl*>((NodeImpl*)_parent);
}

SharedPtr<GraphObject>
GraphObjectImpl::find_child(const std::string& name) const
{
	throw;
}

} // namespace Server
} // namespace Ingen