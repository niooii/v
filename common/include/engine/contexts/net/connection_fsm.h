//
// Created by niooi on 10/11/2025.
//

#pragma once

#include <defs.h>
#include <hfsm2/machine.hpp>
#include <time/stopwatch.h>
#include "moodycamel/concurrentqueue.h"

extern "C" {
#include <enet.h>
}

namespace v {
    class NetConnection;
    class NetworkContext;

    // Context shared across connection states
    struct ConnectionContext {
        NetConnection* connection;
        NetworkContext* net_ctx;
        ENetPeer* peer;
        f64 connection_timeout;

        Stopwatch since_open{};

        // Packets queued before connection is active
        moodycamel::ConcurrentQueue<ENetPacket*> pending_packets{};
        moodycamel::ConcurrentQueue<ENetPacket*>* outgoing_packets{nullptr};

        bool remote_disconnected{false};
    };

    // FSM configuration
    using ConnectionFSMConfig = hfsm2::Config::ContextT<ConnectionContext&>;
    using ConnectionFSM = hfsm2::MachineT<ConnectionFSMConfig>;

    // Forward declarations
    struct Connecting;
    struct Active;
    struct Disconnecting;

    // FSM structure - flat, no hierarchy needed
    using ConnectionFSMRoot = ConnectionFSM::Root<
        Connecting,
        Active,
        Disconnecting
    >;

    // State implementations
    struct Connecting : ConnectionFSMRoot::State {
        void enter(ConnectionFSMRoot::Control& control);

        void update(ConnectionFSMRoot::FullControl& control);
    };

    struct Active : ConnectionFSMRoot::State {
        void enter(ConnectionFSMRoot::Control& control);

        void update(ConnectionFSMRoot::FullControl& control);
    };

    struct Disconnecting : ConnectionFSMRoot::State {
        void enter(ConnectionFSMRoot::Control& control);
    };

} // namespace v
