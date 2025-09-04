//
// Created by niooi on 7/31/2025.
//

#pragma once

#include <defs.h>
#include <enet.h>

namespace v {
    class NetConnection;
    typedef std::function<void(std::shared_ptr<NetConnection>)> OnConnectCallback;

    class NetListener {
        friend class NetworkContext;
    public:
        /// Set a callback to run when a new incoming connection is created.
        /// If new_only is false, the callback will run immediately for 
        /// all the incoming connections that already exist. 
        /// If new_only is true, the callback will only run for *future* connections. 
        /// new_only is false by default.
        void on_connection(OnConnectCallback callback, bool new_only = false);

    private:
        NetListener(class NetworkContext* ctx, const std::string& host, u16 port, u32 max_connections = 128);

        std::string addr_;
        u16 port_;

        NetworkContext* net_ctx_;
        ENetHost* host_;

        std::vector<OnConnectCallback> conn_callbacks_;
    };
}
