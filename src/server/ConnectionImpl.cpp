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

#include <algorithm>
#include "raul/log.hpp"
#include "raul/Maid.hpp"
#include "raul/IntrusivePtr.hpp"
#include "shared/LV2URIMap.hpp"
#include "AudioBuffer.hpp"
#include "BufferFactory.hpp"
#include "ConnectionImpl.hpp"
#include "Engine.hpp"
#include "EventBuffer.hpp"
#include "InputPort.hpp"
#include "MessageContext.hpp"
#include "OutputPort.hpp"
#include "PortImpl.hpp"
#include "ProcessContext.hpp"
#include "mix.hpp"
#include "util.hpp"

namespace Ingen {
namespace Server {

/** Constructor for a connection from a node's output port.
 *
 * This handles both polyphonic and monophonic nodes, transparently to the
 * user (InputPort).
 */
ConnectionImpl::ConnectionImpl(BufferFactory& bufs, PortImpl* src_port, PortImpl* dst_port)
	: _queue(NULL)
	, _bufs(bufs)
	, _src_port(src_port)
	, _dst_port(dst_port)
	, _pending_disconnection(false)
{
	assert(src_port);
	assert(dst_port);
	assert(src_port != dst_port);
	assert(src_port->path() != dst_port->path());

	if (must_queue())
		_queue = new Raul::RingBuffer(src_port->buffer_size() * 2);
}

void
ConnectionImpl::dump() const
{
	debug << _src_port->path() << " -> " << _dst_port->path()
		<< (must_mix()   ? " (MIX) " : " (DIRECT) ")
		<< (must_queue() ? " (QUEUE)" : " (NOQUEUE) ")
		<< "POLY: " << _src_port->poly() << " => " << _dst_port->poly() << endl;
}

void
ConnectionImpl::get_sources(Context& context, uint32_t voice,
		IntrusivePtr<Buffer>* srcs, uint32_t max_num_srcs, uint32_t& num_srcs)
{
	if (must_queue() && _queue->read_space() > 0) {
		LV2_Atom obj;
		_queue->peek(sizeof(LV2_Atom), &obj);
		IntrusivePtr<Buffer> buf = context.engine().buffer_factory()->get(
				dst_port()->buffer_type(), sizeof(LV2_Atom) + obj.size);
		void* data = buf->port_data(PortType::MESSAGE, context.offset());
		_queue->read(sizeof(LV2_Atom) + obj.size, (LV2_Atom*)data);
		srcs[num_srcs++] = buf;
	} else if (must_mix()) {
		// Mixing down voices: every src voice mixed into every dst voice
		for (uint32_t v = 0; v < _src_port->poly(); ++v) {
			assert(num_srcs < max_num_srcs);
			srcs[num_srcs++] = _src_port->buffer(v).get();
		}
	} else {
		// Matching polyphony: each src voice mixed into corresponding dst voice
		assert(_src_port->poly() == _dst_port->poly());
		assert(num_srcs < max_num_srcs);
		srcs[num_srcs++] = _src_port->buffer(voice).get();
	}
}

void
ConnectionImpl::queue(Context& context)
{
	if (!must_queue())
		return;

	IntrusivePtr<EventBuffer> src_buf = PtrCast<EventBuffer>(_src_port->buffer(0));
	if (!src_buf) {
		error << "Queued connection but source is not an EventBuffer" << endl;
		return;
	}

	for (src_buf->rewind(); src_buf->is_valid(); src_buf->increment()) {
		LV2_Event* ev  = src_buf->get_event();
		LV2_Atom*  obj = LV2_ATOM_FROM_EVENT(ev);
		/*debug << _src_port->path() << " -> " << _dst_port->path()
			<< " QUEUE OBJECT TYPE " << obj->type << ":";
		for (size_t i = 0; i < obj->size; ++i)
			debug << " " << std::hex << (int)obj->body[i];
		debug << endl;*/

		_queue->write(sizeof(LV2_Atom) + obj->size, obj);
		context.engine().message_context()->run(_dst_port->parent_node(), context.start() + ev->frames);
	}
}

bool
ConnectionImpl::can_connect(const OutputPort* src, const InputPort* dst)
{
	const Ingen::Shared::LV2URIMap& uris = src->bufs().uris();
	return (
			// (Audio | Control) => (Audio | Control)
			(   (src->is_a(PortType::CONTROL) || src->is_a(PortType::AUDIO))
			 && (dst->is_a(PortType::CONTROL) || dst->is_a(PortType::AUDIO)))

			// (Events | Message) => (Events | Message)
			|| ( (src->is_a(PortType::EVENTS) || src->is_a(PortType::MESSAGE))
			  && (dst->is_a(PortType::EVENTS) || dst->is_a(PortType::MESSAGE)))

			// (Message | Value) => (Message | Value)
			|| ( (src->is_a(PortType::MESSAGE) || src->is_a(PortType::VALUE))
			  && (dst->is_a(PortType::MESSAGE) || dst->is_a(PortType::VALUE)))

			// Control => atom:Float32 Value
			|| (src->is_a(PortType::CONTROL) && dst->supports(uris.atom_Float32))

			// Audio => atom:Vector Value
			|| (src->is_a(PortType::AUDIO)   && dst->supports(uris.atom_Vector))

			// atom:Float32 Value => Control
			|| (src->supports(uris.atom_Float32) && dst->is_a(PortType::CONTROL))

			// atom:Vector Value => Audio
			|| (src->supports(uris.atom_Vector)  && dst->is_a(PortType::AUDIO)));
}

} // namespace Server
} // namespace Ingen
