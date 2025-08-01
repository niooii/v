//
// Created by niooi on 7/31/2025.
//

#pragma once

#include <domain/context.h>
#include <string>
#include <entt/entt.h>

extern "C" {
    #include <enet.h>
}

namespace v {
	typedef ChannelId = u64;
	// TODO! PLACEHOLDER what type shouldbuffer be
	typedef Buffer = u16;
	// TODO! move this out somewhere
	typedef DomainId = entt::entity;

	/// A component that represents a network communication channel
	class ConnectionChannel {
	friend class ConnectionDomain;	
	public:
		/// Accepts the channel id, domain id of the ConnectionDomain and domain id of the owner
		// TODO! this pretty much locks one domain into having one connection channel. what if they want multiple?
		// think on this. plus who owns the component, ConnDomain or the calling domain? 
		// this is kinda horrid...
		entt::sink<entt::sigh<void(ChannelId, DomainId, DomainId)>> on_disconnect;
		entt::sink<entt::sigh<void(ChannelId, DomainId, Buffer)>> on_recv;
			
	private:
		ConnectionChannel(DomainId domain_id, ChannelId channel_id);
		// Called from ConnectionDomain
		void update(Buffer data);
		
		ChannelId id_;
	};

	/// A domain that represents a connection between a host and a peer 
	class ConnectionDomain : public Domain {
	friend class NetworkContext;
	public:
		ConnectionChannel* create_channel(ChannelId channel_id); 
		
	private:
		ConnectionDomain(Engine& engine, ENetPeer* peer);
		/// Routes the data to it's specified channel.  Called from NetworkContext. 
		void update(Buffer data);
		
		Map<ChannelId, ConnectionChannel> channels_;
	};

	/// A context that creates and manages ConnectionDomains
    /// NetworkContext is thread-safe.
    class NetworkContext : public Context {
    public:
    	// TODO! change this to accept a Option<HostOptions> struct, if not provided then it will not
    	// bind the host to any port/addr. (client usage, vs server usage where you will pass in a host struct)
        explicit NetworkContext(Engine& engine, const std::string& host_ip = "0.0.0.0", u16 port = 7777);
        ~NetworkContext();

		/// Fires when a new connection is creataed. 
        entt::sink<entt::sigh<void(DomainId)>> on_conn;

		// TODO! this kinda works for client to server but what abotu server accepting clients?
		// auto create connectiondomain??? probably.
		ConnectionDomain* create_connection(const std::string_view host_ip, u16 port);

		/// Handles network flushing and updating  
        void update();
        
    private:
        ENetHost* host_;
        Map<ENetPeer*, ConnectionDomain> domains_;
    };
}
