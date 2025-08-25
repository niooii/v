//
// Created by niooi on 7/31/2025.
//

#pragma once

#include <absl/debugging/internal/demangle.h>
#include <defs.h>
#include <domain/context.h>
#include <entt/entt.hpp>
#include <string>
#include "entt/entity/fwd.hpp"

extern "C" {
#include <enet.h>
}

namespace v {
    // TODO! think about how to design this for fast use from multiple threads.

    typedef u16 Channel;

    /// An abstract channel class, created and managed by a NetConnection connection
    /// object, meaning a channel is unique to a specific NetConnection only. To create a
    /// channel, inherit from this class. E.g. class ChatChannel : public NetChannel {};
    class NetChannel {
    public:
        /// Get the owning NetConnection.
        FORCEINLINE const class NetConnection* connection_info() { return conn_; };
        /// Assigns a unique name to the channel, to be used for communication
        /// over a network.
        /// By default assigns the name of the struct and it's namespaces
        /// (e.g. "v::Chat::ChatChannel").
        /// Override this to set a new name that may be more consistent through codebase
        /// and version changes.
        /// TODO! check if default impl guarentees uniqueness
        /// TODO! find a way to enforce uniqueness for custom names, or just not allow
        /// inheritance
        virtual FORCEINLINE const std::string_view unique_name()
        {
            return std::string_view{ typename_ };
        }

    private:
        NetChannel(NetConnection* conn) :
            conn_(conn),
            typename_(absl::debugging_internal::DemangleString(typeid(this).name())) {};

        const NetConnection* conn_;
        const std::string    typename_;
    };

    template <typename T>
    concept DerivedFromChannel = std::is_base_of_v<NetChannel, T>;

    class NetConnection {
    public:
        /// Creates a NetChannel for isolated network communication
        template <DerivedFromChannel T>
        NetChannel* create_channel();

    private:
        NetConnection(const std::string& host, u16 port);
        entt::entity entity_;
    };

    /// A context that creates and manages network connections.
    /// NetworkContext is thread-safe.
    class NetworkContext : public Context {
    public:
        // TODO! change this to accept a Option<HostOptions> struct, if not provided then
        // it will not bind the host to any port/addr. (client usage, vs server usage
        // where you will pass in a host struct)
        explicit NetworkContext(Engine& engine);
        ~NetworkContext();

        /// Fires when a new connection is creataed.
        // entt::sink<entt::sigh<void(DomainId)>> on_conn;

        // TODO! this kinda works for client to server but what abotu server accepting
        // clients? auto create connectiondomain??? probably. ConnectionDomain*
        // create_connection(const std::string_view host_ip, u16 port);

        /// Handles network flushing and updating
        void update();

    private:
        // Don't use the main engine domain registry.
        entt::registry reg_;

        ENetHost* host_;
        // Map<ENetPeer*, ConnectionDomain> domains_;
    };
    // 	typedef ChannelId = u64;
    // 	// TODO! PLACEHOLDER what type shouldbuffer be
    // 	typedef Buffer = u16;
    // 	// TODO! move this out somewhere
    // 	typedef DomainId = entt::entity;
    //
    // 	/// A component that represents a network communication channel
    // 	class ConnectionChannel {
    // 	friend class ConnectionDomain;
    // 	public:
    // 		/// Accepts the channel id, domain id of the ConnectionDomain and domain id of
    // the owner
    // 		// TODO! this pretty much locks one domain into having one connection channel.
    // what if they want multiple?
    // 		// think on this. plus who owns the component, ConnDomain or the calling
    // domain?
    // 		// this is kinda horrid...
    // 		entt::sink<entt::sigh<void(ChannelId, DomainId, DomainId)>> on_disconnect;
    // 		entt::sink<entt::sigh<void(ChannelId, DomainId, Buffer)>> on_recv;
    //
    // 	private:
    // 		ConnectionChannel(DomainId domain_id, ChannelId channel_id);
    // 		// Called from ConnectionDomain
    // 		void update(Buffer data);
    //
    // 		ChannelId id_;
    // 	};
    //
    // 	/// A domain that represents a connection between a host and a peer
    // 	class ConnectionDomain : public Domain {
    // 	friend class NetworkContext;
    // 	public:
    // 		ConnectionChannel* create_channel(ChannelId channel_id);
    //
    // 	private:
    // 		ConnectionDomain(Engine& engine, ENetPeer* peer);
    // 		/// Routes the data to it's specified channel.  Called from NetworkContext.
    // 		void update(Buffer data);
    //
    // 		Map<ChannelId, ConnectionChannel> channels_;
    // 	};
    //
} // namespace v
