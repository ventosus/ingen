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

#include "AllNotesOffEvent.hpp"
#include "Responder.hpp"
#include "Engine.hpp"
#include "EngineStore.hpp"
#include "module/World.hpp"
#include "shared/Store.hpp"

namespace Ingen {


/** Note off with patch explicitly passed - triggered by MIDI.
 */
AllNotesOffEvent::AllNotesOffEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, PatchImpl* patch)
: Event(engine, responder, timestamp),
  _patch(patch)
{
}


/** Note off event with lookup - triggered by OSC.
 */
AllNotesOffEvent::AllNotesOffEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& patch_path)
: Event(engine, responder, timestamp),
  _patch_path(patch_path),
  _patch(NULL)
{
}


void
AllNotesOffEvent::execute(ProcessContext& context)
{
	Event::execute(context);

	if (_patch == NULL && _patch_path != "")
		_patch = _engine.engine_store()->find_patch(_patch_path);
		
	//if (_patch != NULL)
	//	for (Raul::List<MidiInNode*>::iterator j = _patch->midi_in_nodes().begin(); j != _patch->midi_in_nodes().end(); ++j)
	//		(*j)->all_notes_off(offset);
}


void
AllNotesOffEvent::post_process()
{
	if (_patch != NULL)
		_responder->respond_ok();
}


} // namespace Ingen

