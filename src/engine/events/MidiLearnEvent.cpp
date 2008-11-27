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

#include "MidiLearnEvent.hpp"
#include "Responder.hpp"
#include "Engine.hpp"
#include "EngineStore.hpp"
#include "NodeImpl.hpp"
#include "MidiControlNode.hpp"
#include "ClientBroadcaster.hpp"
#include "PluginImpl.hpp"

namespace Ingen {


// MidiLearnResponseEvent

void
MidiLearnResponseEvent::post_process()
{
	_engine.broadcaster()->send_port_value(_port_path, _value);
}



// MidiLearnEvent

MidiLearnEvent::MidiLearnEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& node_path)
	: QueuedEvent(engine, responder, timestamp)
	, _error(NO_ERROR)
	, _node_path(node_path)
	, _node(NULL)
	, _response_event(NULL)
{
}


void
MidiLearnEvent::pre_process()
{
	_node = _engine.engine_store()->find_node(_node_path);
	_response_event = new MidiLearnResponseEvent(_engine, _node_path + "/Controller_Number", _time);
	
	QueuedEvent::pre_process();
}


void
MidiLearnEvent::execute(ProcessContext& context)
{	
	QueuedEvent::execute(context);
	
	if (_node != NULL) {
	   if (_node->plugin_impl()->type() == Plugin::Internal
				&& _node->plugin_impl()->uri() == "http://drobilla.net/ns/ingen#control_node") {
			((MidiControlNode*)_node)->learn(_response_event);
	   } else {
		   std::cout << "NOT CAPABLE: " << _node->plugin_impl()->uri() << std::endl;
		   _error = INVALID_NODE_TYPE;
	   }
	}
}


void
MidiLearnEvent::post_process()
{
	if (_error == NO_ERROR) {
		_responder->respond_ok();
	} else if (_node == NULL) {
		string msg = "Did not find node '";
		msg.append(_node_path).append("' for MIDI learn.");
		_responder->respond_error(msg);
	} else {
		const string msg = string("Node '") + _node_path + "' is not capable of MIDI learn.";
		_responder->respond_error(msg);
	}
}


} // namespace Ingen


