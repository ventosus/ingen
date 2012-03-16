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

#ifndef INGEN_CLIENT_NODEMODEL_HPP
#define INGEN_CLIENT_NODEMODEL_HPP

#include <cstdlib>
#include <string>
#include <vector>

#include "raul/SharedPtr.hpp"

#include "ingen/Node.hpp"
#include "ingen/Port.hpp"
#include "ingen/client/ObjectModel.hpp"
#include "ingen/client/PortModel.hpp"
#include "ingen/client/PluginModel.hpp"

namespace Raul { class Path; }

namespace Ingen {

namespace Shared { class URIs; }

namespace Client {

class PluginModel;
class ClientStore;

/** Node model class, used by the client to store engine's state.
 *
 * \ingroup IngenClient
 */
class NodeModel : public ObjectModel,
                  virtual public Ingen::Node
{
public:
	NodeModel(const NodeModel& copy);
	virtual ~NodeModel();

	typedef std::vector< SharedPtr<const PortModel> > Ports;

	SharedPtr<const PortModel> get_port(const Raul::Symbol& symbol) const;

	Port* port(uint32_t index) const;

	const Raul::URI&       plugin_uri()   const { return _plugin_uri; }
	const Plugin*          plugin()       const { return _plugin.get(); }
	Plugin*                plugin()             { return _plugin.get(); }
	SharedPtr<PluginModel> plugin_model() const { return _plugin; }
	uint32_t               num_ports()    const { return _ports.size(); }
	const Ports&           ports()        const { return _ports; }

	void default_port_value_range(SharedPtr<const PortModel> port,
	                              float& min, float& max, uint32_t srate=1) const;

	void port_value_range(SharedPtr<const PortModel> port,
	                      float& min, float& max, uint32_t srate=1) const;

	std::string port_label(SharedPtr<const PortModel> port) const;

	// Signals
	INGEN_SIGNAL(new_port, void, SharedPtr<const PortModel>);
	INGEN_SIGNAL(removed_port, void, SharedPtr<const PortModel>);

protected:
	friend class ClientStore;

	NodeModel(Shared::URIs&          uris,
	          const Raul::URI&       plugin_uri,
	          const Raul::Path&      path);
	NodeModel(Shared::URIs&          uris,
	          SharedPtr<PluginModel> plugin,
	          const Raul::Path&      path);
	NodeModel(const Raul::Path& path);

	void add_child(SharedPtr<ObjectModel> c);
	bool remove_child(SharedPtr<ObjectModel> c);
	void add_port(SharedPtr<PortModel> pm);
	void remove_port(SharedPtr<PortModel> pm);
	void remove_port(const Raul::Path& port_path);
	void add_program(int bank, int program, const std::string& name);
	void remove_program(int bank, int program);
	void set(SharedPtr<ObjectModel> model);

	virtual void clear();

	Ports                  _ports; ///< Vector of ports (not a Table to preserve order)
	Raul::URI              _plugin_uri; ///< Plugin URI (if PluginModel is unknown)
	SharedPtr<PluginModel> _plugin; ///< The plugin this node is an instance of

private:
	mutable uint32_t _num_values; ///< Size of _min_values and _max_values
	mutable float*   _min_values; ///< Port min values (cached for LV2)
	mutable float*   _max_values; ///< Port max values (cached for LV2)
};

} // namespace Client
} // namespace Ingen

#endif // INGEN_CLIENT_NODEMODEL_HPP