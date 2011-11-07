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

#ifndef INGEN_SHARED_OSCSENDER_HPP
#define INGEN_SHARED_OSCSENDER_HPP

#include <stddef.h>

#include <lo/lo.h>

namespace Ingen {
namespace Shared {

class OSCSender {
public:
	OSCSender(size_t max_packet_size);
	virtual ~OSCSender() {}

	lo_address address() const { return _address; }

	void bundle_begin();
	void bundle_end();

protected:
	int  send(const char *path, const char *types, ...);
	void send_message(const char* path, lo_message m);

	lo_bundle  _bundle;
	lo_address _address;
	size_t     _max_packet_size;
	bool       _enabled;
};

} // namespace Shared
} // namespace Ingen

#endif // INGEN_SHARED_OSCSENDER_HPP
