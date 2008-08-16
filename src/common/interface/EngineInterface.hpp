/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 * 
 * Ingen is free software = 0; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation = 0; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY = 0; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program = 0; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef ENGINEINTERFACE_H
#define ENGINEINTERFACE_H

#include <inttypes.h>
#include <string>
#include <raul/SharedPtr.hpp>
#include "interface/ClientInterface.hpp"
#include "interface/CommonInterface.hpp"

namespace Ingen {
namespace Shared {


/** The (only) interface clients use to communicate with the engine.
 * Purely virtual (except for the destructor).
 *
 * \ingroup interface
 */
class EngineInterface : public CommonInterface
{
public:
	virtual ~EngineInterface() {}
	
	// Responses
	virtual void set_next_response_id(int32_t id) = 0;
	virtual void disable_responses() = 0;
	
	// Client registration
	virtual void register_client(ClientInterface* client) = 0;
	virtual void unregister_client(const std::string& uri) = 0;
	
	// Engine commands
	virtual void load_plugins() = 0;
	virtual void activate() = 0;
	virtual void deactivate() = 0;
	virtual void quit() = 0;
	
	// Bundles
	virtual void bundle_begin() = 0;
	virtual void bundle_end()   = 0;
	
	// Object commands
	
	virtual void new_patch(const std::string& path,
	                       uint32_t           poly) = 0;
	
	virtual void create_port(const std::string& path,
	                         const std::string& data_type,
	                         bool               is_output) = 0;
	
	virtual void create_node(const std::string& path,
	                         const std::string& plugin_uri,
	                         bool               polyphonic) = 0;
	
	/** DEPRECATED */
	virtual void create_node(const std::string& path,
	                         const std::string& plugin_type,
	                         const std::string& library_name,
	                         const std::string& plugin_label,
	                         bool               polyphonic) = 0;
	
	virtual void rename(const std::string& old_path,
	                    const std::string& new_symbol) = 0;
	
	virtual void destroy(const std::string& path) = 0;
	
	virtual void clear_patch(const std::string& patch_path) = 0;

	virtual void set_polyphony(const std::string& patch_path, uint32_t poly) = 0;
	
	virtual void set_polyphonic(const std::string& path, bool poly) = 0;
	
	virtual void enable_patch(const std::string& patch_path) = 0;
	
	virtual void disable_patch(const std::string& patch_path) = 0;
	
	virtual void connect(const std::string& src_port_path,
	                     const std::string& dst_port_path) = 0;
	
	virtual void disconnect(const std::string& src_port_path,
	                        const std::string& dst_port_path) = 0;
	
	virtual void disconnect_all(const std::string& parent_patch_path,
	                            const std::string& path) = 0;
	
	virtual void set_port_value(const std::string& port_path,
	                            const std::string& type_uri,
	                            uint32_t           data_size,
	                            const void*        data) = 0;
	
	virtual void set_port_value(const std::string& port_path,
	                            const std::string& type_uri,
	                            uint32_t           voice,
	                            uint32_t           data_size,
	                            const void*        data) = 0;
	
	virtual void set_port_value_immediate(const std::string& port_path,
	                                      const std::string& type_uri,
	                                      uint32_t           data_size,
	                                      const void*        data) = 0;
	
	virtual void set_port_value_immediate(const std::string& port_path,
	                                      const std::string& type_uri,
	                                      uint32_t           voice,
	                                      uint32_t           data_size,
	                                      const void*        data) = 0;
	
	virtual void enable_port_broadcasting(const std::string& port_path) = 0;
	
	virtual void disable_port_broadcasting(const std::string& port_path) = 0;
	
	virtual void set_program(const std::string& node_path,
	                         uint32_t           bank,
	                         uint32_t           program) = 0;
	
	virtual void midi_learn(const std::string& node_path) = 0;
	
	virtual void set_variable(const std::string& subject_path,
	                          const std::string& predicate,
	                          const Raul::Atom&  value) = 0;
	
	// Requests
	
	virtual void ping() = 0;
	
	virtual void request_plugin(const std::string& uri) = 0;

	virtual void request_object(const std::string& path) = 0;

	virtual void request_port_value(const std::string& port_path) = 0;
	
	virtual void request_variable(const std::string& path,
	                              const std::string& key) = 0;

	virtual void request_plugins() = 0;
	
	virtual void request_all_objects() = 0;
	
protected:
	EngineInterface() {}
};


} // namespace Shared
} // namespace Ingen

#endif // ENGINEINTERFACE_H

