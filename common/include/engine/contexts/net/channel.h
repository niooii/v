//
// Created by niooi on 7/31/2025.
//

#pragma once

#include <defs.h>
#include <engine/contexts/net/connection.h>

namespace v {
    /// An abstract channel class, created and managed by a NetConnection connection
    /// object, meaning a channel is unique to a specific NetConnection only. To create a
    /// channel, inherit from this class.
    /// E.g. class ChatChannel : public NetChannel<ChatChannel> {};
    template <typename Derived>
    class NetChannel {
    public:
        // TODO! onrecv and send functions here, on send maybe?

        /// Get the owning NetConnection.
        FORCEINLINE const class NetConnection* connection_info() { return conn_; };

        /// Assigns a unique name to the channel, to be used for communication
        /// over a network.
        /// By default assigns the name of the struct and it's namespaces
        /// (e.g. "v::Chat::ChatChannel").
        /// Override this to set a new name that may be more consistent through codebase
        /// and version changes.
        /// TODO! find a way to enforce uniqueness for custom names, or just not allow
        /// inheritance
        constexpr virtual const std::string_view unique_name()
        {
            return type_name<Derived>();
        }

        void send(char* buf, u64 len);

    private:
        NetChannel(NetConnection* conn) : conn_(conn) {}

        // used for automatic creation of a channel that was opened over a network.
        // do not call this
        void create_self() { conn_->create_channel<Derived>(); }

        const NetConnection* conn_;
    };
} // namespace v
