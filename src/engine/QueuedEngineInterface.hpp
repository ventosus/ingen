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

#ifndef QUEUEDENGINEINTERFACE_H
#define QUEUEDENGINEINTERFACE_H

#include <inttypes.h>
#include <string>
#include <memory>
#include <raul/SharedPtr.hpp>
#include "tuning.hpp"
#include "interface/EngineInterface.hpp"
#include "interface/ClientInterface.hpp"
#include "Responder.hpp"
#include "QueuedEventSource.hpp"
#include "Engine.hpp"
using std::string;

namespace Ingen {

using Shared::ClientInterface;
using Shared::EngineInterface;
class Engine;


/** A queued (preprocessed) event source / interface.
 *
 * This is the bridge between the EngineInterface presented to the client, and
 * the EventSource that needs to be presented to the AudioDriver.
 *
 * Responses occur through the event mechanism (which notified clients in
 * event post_process methods) and are related to an event by an integer ID.
 * If you do not register a responder, you have no way of knowing if your calls
 * are successful.
 */
class QueuedEngineInterface : public QueuedEventSource, public EngineInterface
{
public:
	QueuedEngineInterface(Engine& engine, size_t queued_size, size_t stamped_size);
	virtual ~QueuedEngineInterface() {}
	
	std::string uri() const { return "ingen:internal"; }
	
	void set_next_response_id(int32_t id);

	// Client registration
	virtual void register_client(ClientInterface* client);
	virtual void unregister_client(const string& uri);

	// Engine commands
	virtual void load_plugins();
	virtual void activate();
	virtual void deactivate();
	virtual void quit();
	
	// Bundles
	virtual void bundle_begin();
	virtual void bundle_end();
	
	// Object commands
	
	virtual void new_patch(const string& path,
	                       uint32_t      poly);

	virtual void new_port(const string& path,
	                      uint32_t      index,
	                      const string& data_type,
	                      bool          direction);

	virtual void new_node(const string& path,
	                      const string& plugin_uri);
	
	/** FIXME: DEPRECATED, REMOVE */
	virtual void new_node_deprecated(const string& path,
	                                 const string& plugin_type,
	                                 const string& lib_path,
	                                 const string& plug_label);

	virtual void rename(const string& old_path,
	                    const string& new_name);

	virtual void destroy(const string& path);

	virtual void clear_patch(const string& patch_path);
	
	virtual void set_polyphony(const string& patch_path, uint32_t poly);
	
	virtual void set_polyphonic(const string& path, bool poly);

	virtual void connect(const string& src_port_path,
	                     const string& dst_port_path);

	virtual void disconnect(const string& src_port_path,
	                        const string& dst_port_path);

	virtual void disconnect_all(const string& patch_path,
	                            const string& node_path);

	virtual void set_port_value(const string&     port_path,
	                            const Raul::Atom& value);
	
	virtual void set_voice_value(const string&     port_path,
	                             uint32_t          voice,
	                             const Raul::Atom& value);
	
	virtual void set_program(const string& node_path,
	                         uint32_t      bank,
	                         uint32_t      program);

	virtual void midi_learn(const string& node_path);

	virtual void set_variable(const string&     path,
	                          const string&     predicate,
	                          const Raul::Atom& value);
	
	virtual void set_property(const string&     path,
	                          const string&     predicate,
	                          const Raul::Atom& value);
	
	// Requests //
	
	virtual void ping();
	virtual void request_plugin(const string& uri);
	virtual void request_object(const string& path);
	virtual void request_port_value(const string& port_path);
	virtual void request_variable(const string& object_path, const string& key);
	virtual void request_property(const string& object_path, const string& key);
	virtual void request_plugins();
	virtual void request_all_objects();

protected:
	
	virtual void disable_responses();

	SharedPtr<Responder> _responder; ///< NULL if responding disabled
	Engine&              _engine;
	bool                 _in_bundle; ///< True iff a bundle is currently being received

private:
	SampleCount now() const;
};


} // namespace Ingen

extern "C" {
	extern Ingen::QueuedEngineInterface* new_queued_interface(Ingen::Engine& engine);
}

#endif // QUEUEDENGINEINTERFACE_H
