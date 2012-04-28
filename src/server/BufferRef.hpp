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

#ifndef INGEN_ENGINE_BUFFERREF_HPP
#define INGEN_ENGINE_BUFFERREF_HPP

#include <boost/intrusive_ptr.hpp>

namespace Ingen {
namespace Server {

class Buffer;

typedef boost::intrusive_ptr<Buffer> BufferRef;

} // namespace Server
} // namespace Ingen

namespace boost {

inline void intrusive_ptr_add_ref(Ingen::Server::Buffer* b) { b->ref(); }
inline void intrusive_ptr_release(Ingen::Server::Buffer* b) { b->deref(); }

}

#endif // INGEN_ENGINE_BUFFERREF_HPP