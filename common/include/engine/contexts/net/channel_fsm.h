//
// Created by niooi on 10/11/2025.
//

#pragma once

#include <defs.h>
#include <hfsm2/machine.hpp>
#include <memory>
#include <string>
#include "moodycamel/concurrentqueue.h"

extern "C" {
#include <enet.h>
}

namespace v {
    class NetChannelBase;

    // Context shared across channel states
    struct ChannelContext {
        std::string name;
        u32 remote_uid{0};
        NetChannelBase* local_channel{nullptr};
        std::unique_ptr<moodycamel::ConcurrentQueue<ENetPacket*>> packet_queue{};
    };

    // FSM configuration
    using ChannelFSMConfig = hfsm2::Config::ContextT<ChannelContext&>;
    using ChannelFSM = hfsm2::MachineT<ChannelFSMConfig>;

    // Forward declarations
    struct LocalOnly;
    struct RemoteOnly;
    struct Linked;

    // FSM structure - flat states representing channel sync status
    using ChannelFSMRoot = ChannelFSM::Root<
        LocalOnly,
        RemoteOnly,
        Linked
    >;

    // State implementations
    struct LocalOnly : ChannelFSMRoot::State {
        // Channel created locally, waiting for remote CHANNEL| packet
        void enter(ChannelFSMRoot::Control& control);

        // Queue incoming packets until remote connects
        void handle_packet(ENetPacket* packet, ChannelFSMRoot::Control& control);
    };

    struct RemoteOnly : ChannelFSMRoot::State {
        // Received CHANNEL| packet, waiting for local create_channel()
        void enter(ChannelFSMRoot::Control& control);

        // Queue incoming packets until local channel exists
        void handle_packet(ENetPacket* packet, ChannelFSMRoot::Control& control);
    };

    struct Linked : ChannelFSMRoot::State {
        // Both local and remote channels exist
        void enter(ChannelFSMRoot::Control& control);

        // Route packets directly to local channel
        void handle_packet(ENetPacket* packet, ChannelFSMRoot::Control& control);
    };

} // namespace v
