// Simple integration test for NetworkContext

#include <engine/contexts/net/connection.h>
#include <engine/contexts/net/ctx.h>
#include <engine/contexts/net/listener.h>
#include <net/channels.h>
#include <testing.h>
#include <time/time.h>

using namespace v;

int main()
{
    auto [engine, tctx] = testing::init_test("net");

    // network context and local loopback server + client
    auto* net = engine->add_ctx<NetworkContext>(*engine, 1.0 / 1000.0);

    const std::string host = "127.0.0.1";
    const u16         port = 28555;

    auto listener = net->listen_on(host, port);

    std::atomic_bool on_connect{ false };
    std::atomic_bool server_got_chat{ false };
    std::atomic_bool client_got_echo{ false };

    // server component for connect + chat
    auto& server_comp      = listener->create_component(engine->entity());
    server_comp.on_connect = [&](std::shared_ptr<NetConnection> con)
    {
        on_connect = true;

        auto& chat = con->create_channel<ChatChannel>();
        auto& cc   = chat.create_component(engine->entity());
        cc.on_recv = [&, eng = engine.get()](const ChatMessage& msg)
        {
            server_got_chat = true;
            // echo message back to all channels
            for (auto [e, channel] : eng->view<ChatChannel>().each())
            {
                ChatMessage payload{ .msg = msg.msg };
                channel->send(payload);
            }
        };
    };

    // create client connection
    auto  client = net->create_connection(host, port);
    auto& cchat  = client->create_channel<ChatChannel>();
    auto& ccc    = cchat.create_component(engine->entity());
    ccc.on_recv  = [&](const ChatMessage& msg)
    {
        if (msg.msg == "ping")
            client_got_echo = true;
    };

    bool sent_ping = false;

    constexpr u64 max_ticks = 2000; // ~ test budget
    for (u64 i = 0; i < max_ticks; ++i)
    {
        // drive network and engine
        net->update();
        engine->tick();

        // send once when we think connection is likely active
        if (!sent_ping && on_connect.load())
        {
            ChatMessage msg{ .msg = "ping" };
            cchat.send(msg);
            sent_ping = true;
        }

        // progress checks with deadlines
        tctx.expect_before(on_connect.load(), 400, "server accepted client");
        tctx.expect_before(server_got_chat.load(), 800, "server received chat");
        tctx.expect_before(client_got_echo.load(), 1200, "client received echo");

        // sleep main thread for 1ms, so at most 1000 ticks per sec
        v::time::sleep_ms(1);
    }

    // final hard asserts
    tctx.assert_now(on_connect.load(), "on_connect fired");
    tctx.assert_now(server_got_chat.load(), "server received chat");
    tctx.assert_now(client_got_echo.load(), "client received echo");

    return tctx.is_failure();
}
