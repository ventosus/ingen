/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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

#include "raul/Array.hpp"
#include "raul/Atom.hpp"
#include "raul/List.hpp"
#include "raul/Maid.hpp"
#include "raul/Path.hpp"
#include "Responder.hpp"
#include "CreatePort.hpp"
#include "PatchImpl.hpp"
#include "PluginImpl.hpp"
#include "Engine.hpp"
#include "PatchImpl.hpp"
#include "QueuedEventSource.hpp"
#include "EngineStore.hpp"
#include "ClientBroadcaster.hpp"
#include "PortImpl.hpp"
#include "AudioDriver.hpp"
#include "MidiDriver.hpp"
#include "OSCDriver.hpp"
#include "DuplexPort.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Events {

using namespace Shared;


CreatePort::CreatePort(
		Engine&                     engine,
		SharedPtr<Responder>        responder,
		SampleCount                 timestamp,
		const Raul::Path&           path,
		const Raul::URI&            type,
		bool                        is_output,
		QueuedEventSource*          source,
		const Resource::Properties& properties)
	: QueuedEvent(engine, responder, timestamp, true, source)
	, _error(NO_ERROR)
	, _path(path)
	, _type(type)
	, _is_output(is_output)
	, _data_type(type)
	, _patch(NULL)
	, _patch_port(NULL)
	, _driver_port(NULL)
	, _properties(properties)
{
	/* This is blocking because of the two different sets of Patch ports, the array used in the
	 * audio thread (inherited from NodeBase), and the arrays used in the pre processor thread.
	 * If two add port events arrive in the same cycle and the second pre processes before the
	 * first executes, bad things happen (ports are lost).
	 *
	 * TODO: fix this using RCU?
	 */

	if (_data_type == DataType::UNKNOWN) {
		cerr << "[CreatePortEvent] Unknown port type " << type << endl;
		_error = UNKNOWN_TYPE;
	}
}


void
CreatePort::pre_process()
{
	if (_error == UNKNOWN_TYPE || _engine.engine_store()->find_object(_path)) {
		QueuedEvent::pre_process();
		return;
	}

	_patch = _engine.engine_store()->find_patch(_path.parent());

	if (_patch != NULL) {
		assert(_patch->path() == _path.parent());

		size_t buffer_size = 1;
		if (_type.str() != "ingen:Float")
			buffer_size = _engine.audio_driver()->buffer_size();

		const uint32_t old_num_ports = _patch->num_ports();

		_patch_port = _patch->create_port(_path.name(), _data_type, buffer_size, _is_output);
		_patch_port->set_property("rdf:instanceOf", Atom(Atom::URI, _patch_port->meta_uri().str()));
		_patch_port->meta().properties().insert(_properties.begin(), _properties.end());

		if (_patch_port) {

			if (_is_output)
				_patch->add_output(new Raul::List<PortImpl*>::Node(_patch_port));
			else
				_patch->add_input(new Raul::List<PortImpl*>::Node(_patch_port));

			if (_patch->external_ports())
				_ports_array = new Raul::Array<PortImpl*>(old_num_ports + 1, *_patch->external_ports());
			else
				_ports_array = new Raul::Array<PortImpl*>(old_num_ports + 1, NULL);

			_ports_array->at(_patch->num_ports()-1) = _patch_port;
			_engine.engine_store()->add(_patch_port);

			if (!_patch->parent()) {
				if (_type.str() == "lv2:AudioPort") {
					_driver_port = _engine.audio_driver()->create_port(
							dynamic_cast<DuplexPort*>(_patch_port));
				} else if (_type.str() == "lv2ev:EventPort") {
					_driver_port = _engine.midi_driver()->create_port(
							dynamic_cast<DuplexPort*>(_patch_port));
				}
			}

			assert(_ports_array->size() == _patch->num_ports());

		} else {
			_error = CREATION_FAILED;
		}
	}
	QueuedEvent::pre_process();
}


void
CreatePort::execute(ProcessContext& context)
{
	QueuedEvent::execute(context);

	if (_patch_port) {
		_engine.maid()->push(_patch->external_ports());
		_patch->external_ports(_ports_array);
	}

	if (_driver_port) {
		if (_type.str() == "lv2:AudioPort") {
			_engine.audio_driver()->add_port(_driver_port);
		} else if (_type.str() == "lv2ev:EventPort") {
			_engine.midi_driver()->add_port(_driver_port);
		}
	}
}


void
CreatePort::post_process()
{
	string msg;
	switch (_error) {
	case NO_ERROR:
		_responder->respond_ok();
		_engine.broadcaster()->send_object(_patch_port, true);
		break;
	case UNKNOWN_TYPE:
		msg = string("Could not create port ") + _path.str() + " (Unknown type)";
		_responder->respond_error(msg);
		break;
	case CREATION_FAILED:
		msg = string("Could not create port ") + _path.str() + " (Creation failed)";
		_responder->respond_error(msg);
		break;
	}
}


} // namespace Ingen
} // namespace Events
