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

#ifndef INGEN_INTERNALS_NOTE_HPP
#define INGEN_INTERNALS_NOTE_HPP

#include <string>
#include "types.hpp"
#include "NodeImpl.hpp"

namespace Ingen {
namespace Server {

class InputPort;
class OutputPort;
class InternalPlugin;

namespace Internals {

/** MIDI note input node.
 *
 * For pitched instruments like keyboard, etc.
 *
 * \ingroup engine
 */
class NoteNode : public NodeImpl
{
public:
	NoteNode(
			InternalPlugin*    plugin,
			BufferFactory&     bufs,
			const std::string& path,
			bool               polyphonic,
			PatchImpl*         parent,
			SampleRate         srate);

	~NoteNode();

	bool prepare_poly(BufferFactory& bufs, uint32_t poly);
	bool apply_poly(Raul::Maid& maid, uint32_t poly);

	void process(ProcessContext& context);

	void note_on(ProcessContext& context, uint8_t note_num, uint8_t velocity, FrameTime time);
	void note_off(ProcessContext& context, uint8_t note_num, FrameTime time);
	void all_notes_off(ProcessContext& context, FrameTime time);

	void sustain_on(ProcessContext& context, FrameTime time);
	void sustain_off(ProcessContext& context, FrameTime time);

	static InternalPlugin* internal_plugin(Shared::URIs& uris);

private:
	/** Key, one for each key on the keyboard */
	struct Key {
		enum State { OFF, ON_ASSIGNED, ON_UNASSIGNED };
		Key() : state(OFF), voice(0), time(0) {}
		State       state;
		uint32_t    voice;
		SampleCount time;
	};

	/** Voice, one of these always exists for each voice */
	struct Voice {
		enum State { FREE, ACTIVE, HOLDING };
		Voice() : state(FREE), note(0), time(0) {}
		State       state;
		uint8_t     note;
		SampleCount time;
	};

	float note_to_freq(uint8_t num);
	void free_voice(ProcessContext& context, uint32_t voice, FrameTime time);

	Raul::Array<Voice>* _voices;
	Raul::Array<Voice>* _prepared_voices;
	Key                 _keys[128];
	bool                _sustain; ///< Whether or not hold pedal is depressed

	InputPort*  _midi_in_port;
	OutputPort* _freq_port;
	OutputPort* _vel_port;
	OutputPort* _gate_port;
	OutputPort* _trig_port;
};

} // namespace Server
} // namespace Ingen
} // namespace Internals

#endif // INGEN_INTERNALS_NOTE_HPP
