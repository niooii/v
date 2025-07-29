//
// Created by niooi on 7/29/2025.
//

#pragma once

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>
#include <defs.h>
#include <domain/context.h>
#include <glm/vec2.hpp>
#include <input/names.h>

#include <array>
#include <queue>
#include <unordered_dense.h>

namespace v {
    class WindowContext;

    class Window {
        friend class WindowContext;

    public:
        /// Check if key is currently held down
        bool is_key_down(Key key) const;

        /// Check if key was just pressed this frame
        bool is_key_pressed(Key key) const;

        /// Check if key was just released this frame
        bool is_key_released(Key key) const;

        /// Check if mouse button is currently held down
        bool is_mouse_button_down(MouseButton button) const;

        /// Check if mouse button was just pressed this frame
        bool is_mouse_button_pressed(MouseButton button) const;

        /// Check if mouse button was just released this frame
        bool is_mouse_button_released(MouseButton button) const;

        /// Get current mouse position relative to window
        glm::ivec2 get_mouse_position() const;

    private:
        Window(std::string name, glm::ivec2 size, glm::ivec2 pos);
        ~Window();

        void process_event(const SDL_Event& event);

        SDL_Window* sdl_window_;
        glm::ivec2  size_, pos_;
        std::string name_;

        std::array<bool, SDL_SCANCODE_COUNT> curr_keys_;
        std::array<bool, SDL_SCANCODE_COUNT> prev_keys_;
        // SDL supports up to 8 mouse buttons
        std::array<bool, 8> curr_mbuttons;
        std::array<bool, 8> prev_mbuttons;

        glm::ivec2 mouse_position_;
    };

    /// A context for managing windows and input related to windows
    class WindowContext : public Context {
    public:
        WindowContext();
        ~WindowContext();

        /// Create a window with the given parameters
        /// @return A pointer to the window, or nullptr if failure
        Window* create_window(
            const std::string& name, glm::ivec2 size, glm::ivec2 pos);

        /// Destroy a window
        void destroy_window(const Window* window);

        /// Updates windows, pumps events, etc. Should be called at the
        /// desired input update rate, and should be called first in an
        /// application loop to guarentee updated input
        void update();

    private:
        ankerl::unordered_dense::map<Uint32, Window*> windows_;
    };
} // namespace v
