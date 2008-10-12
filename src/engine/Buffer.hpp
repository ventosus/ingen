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

#ifndef BUFFER_H
#define BUFFER_H

#include <cstddef>
#include <cassert>
#include <boost/utility.hpp>
#include <raul/Deletable.hpp>
#include "types.hpp"
#include "interface/DataType.hpp"

namespace Ingen {

	
class Buffer : public boost::noncopyable, public Raul::Deletable
{
public:
	Buffer(Shared::DataType type, size_t size)
		: _type(type)
		, _size(size)
		, _joined_buf(NULL)
	{}
	
	static Buffer* create(Shared::DataType type, size_t size);

	/** Clear contents and reset state */
	virtual void clear() = 0;
	
	virtual void*       raw_data()       = 0;
	virtual const void* raw_data() const = 0;

	/** Rewind (ie reset read pointer), but leave contents unchanged */
	virtual void rewind() const = 0;

	virtual void prepare_read(FrameTime start, SampleCount nframes) = 0;
	virtual void prepare_write(FrameTime start, SampleCount nframes) = 0;
	
	bool is_joined() const { return (_joined_buf != NULL); }
	Buffer* joined_buffer() const { return _joined_buf; }
	
	virtual bool join(Buffer* buf) = 0;
	virtual void unjoin() = 0;
	
	virtual void copy(const Buffer* src, size_t start_sample, size_t end_sample) = 0;

	virtual void resize(size_t size) { _size = size; }

	Shared::DataType type() const { return _type; }
	size_t           size() const { return _size; }

protected:
	Shared::DataType _type;
	size_t           _size;
	Buffer*          _joined_buf;
};


} // namespace Ingen

#endif // BUFFER_H