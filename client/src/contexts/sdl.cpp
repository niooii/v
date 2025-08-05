//
// Created by niooi on 8/5/2025.
//

#include <SDL3/SDL.h>
#include <contexts/sdl.h>
#include <contexts/window.h>

#include "engine/engine.h"

namespace v {
    static bool has_window_id(Uint32 event_type);

    SDLContext::SDLContext(Engine& engine) : Context(engine)
    {
        SDL_InitSubSystem(SDL_INIT_EVENTS);
    }

    SDLContext::~SDLContext()
    {
        SDL_QuitSubSystem(SDL_INIT_EVENTS);
        LOG_INFO("Shutdown SDLContext.");
    }

    void SDLContext::update() const
    {
        SDL_Event event;

        // Poll and route events
        while (SDL_PollEvent(&event))
        {
            if (has_window_id(event.type))
            {
                // Route window events to WindowContext if it exists
                if (auto window_ctx = engine_.get_context<WindowContext>())
                {
                    window_ctx->handle_events(event);
                }
                continue;
            }

            // TODO: Handle global non-window related events here
            // For now, ignore other events besides quit
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                {
                    for (auto [_entity, comp] :
                         engine_.registry_read()->view<SDLComponent>().each())
                        comp.on_quit();
                    break;
                }
            default:;
            }
        }
    }

    SDLComponent& SDLContext::create_component(entt::entity id) const
    {
        return engine_.registry_write()->emplace<SDLComponent>(id);
    }

    static bool has_window_id(Uint32 event_type)
    {
        switch (event_type)
        {
            /* All events that have a windowID */
        case SDL_EVENT_WINDOW_SHOWN:
        case SDL_EVENT_WINDOW_HIDDEN:
        case SDL_EVENT_WINDOW_EXPOSED:
        case SDL_EVENT_WINDOW_MOVED:
        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        case SDL_EVENT_WINDOW_MINIMIZED:
        case SDL_EVENT_WINDOW_MAXIMIZED:
        case SDL_EVENT_WINDOW_RESTORED:
        case SDL_EVENT_WINDOW_MOUSE_ENTER:
        case SDL_EVENT_WINDOW_MOUSE_LEAVE:
        case SDL_EVENT_WINDOW_FOCUS_GAINED:
        case SDL_EVENT_WINDOW_FOCUS_LOST:
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        case SDL_EVENT_WINDOW_HIT_TEST:
        case SDL_EVENT_WINDOW_ICCPROF_CHANGED:
        case SDL_EVENT_WINDOW_DISPLAY_CHANGED:
        case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
        case SDL_EVENT_WINDOW_OCCLUDED:
        case SDL_EVENT_WINDOW_ENTER_FULLSCREEN:
        case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN:
        case SDL_EVENT_WINDOW_DESTROYED:
        case SDL_EVENT_WINDOW_SAFE_AREA_CHANGED:
        case SDL_EVENT_WINDOW_HDR_STATE_CHANGED:
        case SDL_EVENT_WINDOW_METAL_VIEW_RESIZED:

        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
        case SDL_EVENT_TEXT_EDITING:
        case SDL_EVENT_TEXT_INPUT:

        case SDL_EVENT_MOUSE_MOTION:
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
        case SDL_EVENT_MOUSE_WHEEL:

        case SDL_EVENT_DROP_FILE:
        case SDL_EVENT_DROP_TEXT:
        case SDL_EVENT_DROP_BEGIN:
        case SDL_EVENT_DROP_COMPLETE:

        case SDL_EVENT_FINGER_DOWN:
        case SDL_EVENT_FINGER_UP:
        case SDL_EVENT_FINGER_MOTION:
            return true;
        default:
            return false;
        }
    }
} // namespace v
