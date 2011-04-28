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
#include <cmath>
#include <string>
#include "raul/log.hpp"
#include "shared/World.hpp"
#include "shared/LV2URIMap.hpp"
#include "ThreadManager.hpp"
#include "NodeImpl.hpp"
#include "PatchImpl.hpp"
#include "PatchPlugin.hpp"
#include "PortImpl.hpp"
#include "ConnectionImpl.hpp"
#include "DuplexPort.hpp"
#include "Engine.hpp"
#include "ProcessSlave.hpp"
#include "Driver.hpp"
#include "ingen-config.h"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Server {

PatchImpl::PatchImpl(Engine&             engine,
                     const Raul::Symbol& symbol,
                     uint32_t            poly,
                     PatchImpl*          parent,
                     SampleRate          srate,
                     uint32_t            internal_poly)
	: NodeImpl(new PatchPlugin(*engine.world()->uris().get(),
				engine.world()->uris()->ingen_Patch.c_str(), "patch", "Ingen Patch"),
		symbol, poly, parent, srate)
	, _engine(engine)
	, _internal_poly(internal_poly)
	, _compiled_patch(NULL)
	, _process(false)
{
	assert(internal_poly >= 1);
}

PatchImpl::~PatchImpl()
{
	assert(!_activated);

	delete _compiled_patch;
	delete _plugin;
}

void
PatchImpl::activate(BufferFactory& bufs)
{
	NodeImpl::activate(bufs);

	for (List<NodeImpl*>::iterator i = _nodes.begin(); i != _nodes.end(); ++i)
		(*i)->activate(bufs);

	assert(_activated);
}

void
PatchImpl::deactivate()
{
	if (_activated) {
		NodeImpl::deactivate();

		for (List<NodeImpl*>::iterator i = _nodes.begin(); i != _nodes.end(); ++i) {
			if ((*i)->activated())
				(*i)->deactivate();
			assert(!(*i)->activated());
		}
	}
	assert(!_activated);
}

void
PatchImpl::disable()
{
	ThreadManager::assert_thread(THREAD_PROCESS);

	_process = false;

	for (List<PortImpl*>::iterator i = _output_ports.begin(); i != _output_ports.end(); ++i)
		if ((*i)->context() == Context::AUDIO)
			(*i)->clear_buffers();
}

bool
PatchImpl::prepare_internal_poly(BufferFactory& bufs, uint32_t poly)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);

	// TODO: Subpatch dynamic polyphony (i.e. changing port polyphony)

	for (List<NodeImpl*>::iterator i = _nodes.begin(); i != _nodes.end(); ++i)
		(*i)->prepare_poly(bufs, poly);

	for (List<NodeImpl*>::iterator i = _nodes.begin(); i != _nodes.end(); ++i)
		for (uint32_t j = 0; j < (*i)->num_ports(); ++j)
			(*i)->port_impl(j)->prepare_poly_buffers(bufs);

	return true;
}

bool
PatchImpl::apply_internal_poly(ProcessContext& context, BufferFactory& bufs, Raul::Maid& maid, uint32_t poly)
{
	ThreadManager::assert_thread(THREAD_PROCESS);

	// TODO: Subpatch dynamic polyphony (i.e. changing port polyphony)

	for (List<NodeImpl*>::iterator i = _nodes.begin(); i != _nodes.end(); ++i)
		(*i)->apply_poly(maid, poly);

	for (List<NodeImpl*>::iterator i = _nodes.begin(); i != _nodes.end(); ++i) {
		for (uint32_t j = 0; j < (*i)->num_ports(); ++j) {
			PortImpl* const port = (*i)->port_impl(j);
			if (port->is_input() && dynamic_cast<InputPort*>(port)->direct_connect())
				port->setup_buffers(bufs, port->poly());
			port->connect_buffers(context.offset());
		}
	}

	const bool polyphonic = parent_patch() && (poly == parent_patch()->internal_poly());
	for (List<PortImpl*>::iterator i = _output_ports.begin(); i != _output_ports.end(); ++i)
		(*i)->setup_buffers(bufs, polyphonic ? poly : 1);

	_internal_poly = poly;

	return true;
}

/** Run the patch for the specified number of frames.
 *
 * Calls all Nodes in (roughly, if parallel) the order _compiled_patch specifies.
 */
void
PatchImpl::process(ProcessContext& context)
{
	if (!_process)
		return;

	NodeImpl::pre_process(context);

	// Run all nodes
	if (_compiled_patch && _compiled_patch->size() > 0) {
		if (context.slaves().size() > 0) {
			process_parallel(context);
		} else {
			process_single(context);
		}
	}

	// Queue any cross-context connections
	for (CompiledPatch::QueuedConnections::iterator i = _compiled_patch->queued_connections.begin();
			i != _compiled_patch->queued_connections.end(); ++i) {
		(*i)->queue(context);
	}

	NodeImpl::post_process(context);
}

void
PatchImpl::process_parallel(ProcessContext& context)
{
	size_t n_slaves = context.slaves().size();

	CompiledPatch* const cp = _compiled_patch;

	/* Start p-1 slaves */

	if (n_slaves >= cp->size())
		n_slaves = cp->size()-1;

	if (n_slaves > 0) {
		for (size_t i = 0; i < cp->size(); ++i)
			(*cp)[i].node()->reset_input_ready();

		for (size_t i = 0; i < n_slaves; ++i)
			context.slaves()[i]->whip(cp, i+1, context);
	}

	/* Process ourself until everything is done
	 * This is analogous to ProcessSlave::_whipped(), but this is the master
	 * (i.e. what the main Jack process thread calls).  Where ProcessSlave
	 * waits on input, this just skips the node and tries the next, to avoid
	 * waiting in the Jack thread which pisses Jack off.
	 */

	size_t index        = 0;
	size_t num_finished = 0; // Number of consecutive finished nodes hit

	while (num_finished < cp->size()) {
		CompiledNode& n = (*cp)[index];

		if (n.node()->process_lock()) {
			if (n.node()->n_inputs_ready() == n.n_providers()) {
				n.node()->process(context);

				/* Signal dependants their input is ready */
				for (uint32_t i = 0; i < n.dependants().size(); ++i)
					n.dependants()[i]->signal_input_ready();

				++num_finished;
			} else {
				n.node()->process_unlock();
				num_finished = 0;
			}
		} else {
			if (n.node()->n_inputs_ready() == n.n_providers())
				++num_finished;
			else
				num_finished = 0;
		}

		index = (index + 1) % cp->size();
	}

	/* Tell slaves we're done in case we beat them, and pray they're
	 * really done by the start of next cycle.
	 * FIXME: This probably breaks (race) at extremely small nframes where
	 * ingen is the majority of the DSP load.
	 */
	for (uint32_t i = 0; i < n_slaves; ++i)
		context.slaves()[i]->finish();
}

void
PatchImpl::process_single(ProcessContext& context)
{
	for (size_t i = 0; i < _compiled_patch->size(); ++i)
		(*_compiled_patch)[i].node()->process(context);
}

void
PatchImpl::set_buffer_size(Context& context, BufferFactory& bufs, PortType type, size_t size)
{
	NodeImpl::set_buffer_size(context, bufs, type, size);

	for (size_t i = 0; i < _compiled_patch->size(); ++i)
		(*_compiled_patch)[i].node()->set_buffer_size(context, bufs, type, size);
}

// Patch specific stuff

/** Add a node.
 * Preprocessing thread only.
 */
void
PatchImpl::add_node(List<NodeImpl*>::Node* ln)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);
	assert(ln != NULL);
	assert(ln->elem() != NULL);
	assert(ln->elem()->parent_patch() == this);
	//assert(ln->elem()->polyphony() == _internal_poly);

	_nodes.push_back(ln);
}

/** Remove a node.
 * Preprocessing thread only.
 */
PatchImpl::Nodes::Node*
PatchImpl::remove_node(const Raul::Symbol& symbol)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);
	for (List<NodeImpl*>::iterator i = _nodes.begin(); i != _nodes.end(); ++i)
		if ((*i)->symbol() == symbol)
			return _nodes.erase(i);

	return NULL;
}

void
PatchImpl::add_connection(SharedPtr<ConnectionImpl> c)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);
	_connections.insert(make_pair(make_pair(c->src_port(), c->dst_port()), c));
}

/** Remove a connection.
 * Preprocessing thread only.
 */
SharedPtr<ConnectionImpl>
PatchImpl::remove_connection(const PortImpl* src_port, const PortImpl* dst_port)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);
	Connections::iterator i = _connections.find(make_pair(src_port, dst_port));
	if (i != _connections.end()) {
		SharedPtr<ConnectionImpl> c = PtrCast<ConnectionImpl>(i->second);
		_connections.erase(i);
		return c;
	} else {
		error << "[PatchImpl::remove_connection] Connection not found" << endl;
		return SharedPtr<ConnectionImpl>();
	}
}

bool
PatchImpl::has_connection(const PortImpl* src_port, const PortImpl* dst_port) const
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);
	Connections::const_iterator i = _connections.find(make_pair(src_port, dst_port));
	return (i != _connections.end());
}

uint32_t
PatchImpl::num_ports() const
{
	if (ThreadManager::thread_is(THREAD_PROCESS))
		return NodeImpl::num_ports();
	else
		return _input_ports.size() + _output_ports.size();
}

/** Create a port.  Not realtime safe.
 */
PortImpl*
PatchImpl::create_port(BufferFactory& bufs, const string& name, PortType type, size_t buffer_size, bool is_output, bool polyphonic)
{
	if (type == PortType::UNKNOWN) {
		error << "[PatchImpl::create_port] Unknown port type " << type.uri() << endl;
		return NULL;
	}

	assert( !(type == PortType::UNKNOWN) );

	return new DuplexPort(bufs, this, name, num_ports(), polyphonic, _polyphony,
			type, Raul::Atom(), buffer_size, is_output);
}

/** Remove port from ports list used in pre-processing thread.
 *
 * Port is not removed from ports array for process thread (which could be
 * simultaneously running).
 *
 * Realtime safe.  Preprocessing thread only.
 */
List<PortImpl*>::Node*
PatchImpl::remove_port(const string& symbol)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);

	bool found = false;
	List<PortImpl*>::Node* ret = NULL;
	for (List<PortImpl*>::iterator i = _input_ports.begin(); i != _input_ports.end(); ++i) {
		if ((*i)->symbol() == symbol) {
			ret = _input_ports.erase(i);
			found = true;
			break;
		}
	}

	if (!found)
	for (List<PortImpl*>::iterator i = _output_ports.begin(); i != _output_ports.end(); ++i) {
		if ((*i)->symbol() == symbol) {
			ret = _output_ports.erase(i);
			found = true;
			break;
		}
	}

	if ( ! found)
		error << "[PatchImpl::remove_port] Port not found!" << endl;

	return ret;
}

/** Remove all ports from ports list used in pre-processing thread.
 *
 * Ports are not removed from ports array for process thread (which could be
 * simultaneously running).  Returned is a (inputs, outputs) pair.
 *
 * Realtime safe.  Preprocessing thread only.
 */
void
PatchImpl::clear_ports()
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);

	_input_ports.clear();
	_output_ports.clear();
}

Raul::Array<PortImpl*>*
PatchImpl::build_ports_array() const
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);

	Raul::Array<PortImpl*>* const result = new Raul::Array<PortImpl*>(_input_ports.size() + _output_ports.size());

	size_t i = 0;

	for (List<PortImpl*>::const_iterator p = _input_ports.begin(); p != _input_ports.end(); ++p,++i)
		result->at(i) = *p;

	for (List<PortImpl*>::const_iterator p = _output_ports.begin(); p != _output_ports.end(); ++p,++i)
		result->at(i) = *p;

	return result;
}

/** Find the process order for this Patch.
 *
 * The process order is a flat list that the patch will execute in order
 * when it's run() method is called.  Return value is a newly allocated list
 * which the caller is reponsible to delete.  Note that this function does
 * NOT actually set the process order, it is returned so it can be inserted
 * at the beginning of an audio cycle (by various Events).
 *
 * Not realtime safe.
 */
CompiledPatch*
PatchImpl::compile() const
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);

	CompiledPatch* const compiled_patch = new CompiledPatch();//_nodes.size());

	for (Nodes::const_iterator i = _nodes.begin(); i != _nodes.end(); ++i)
		(*i)->traversed(false);

	for (Nodes::const_iterator i = _nodes.begin(); i != _nodes.end(); ++i) {
		NodeImpl* const node = (*i);
		// Either a sink or connected to our output ports:
		if ( ( ! node->traversed()) && node->dependants()->size() == 0)
			compile_recursive(node, compiled_patch);
	}

	// Traverse any nodes we didn't hit yet
	for (Nodes::const_iterator i = _nodes.begin(); i != _nodes.end(); ++i) {
		NodeImpl* const node = (*i);
		if ( ! node->traversed())
			compile_recursive(node, compiled_patch);
	}

	// Add any queued connections that must be run after a cycle
	for (Connections::const_iterator i = _connections.begin(); i != _connections.end(); ++i) {
		SharedPtr<ConnectionImpl> c = PtrCast<ConnectionImpl>(i->second);
		if (c->src_port()->context() == Context::AUDIO &&
				c->dst_port()->context() == Context::MESSAGE) {
			compiled_patch->queued_connections.push_back(c.get());
		}
	}

	assert(compiled_patch->size() == _nodes.size());

	return compiled_patch;
}

} // namespace Server
} // namespace Ingen