//
// Created by niooi on 7/29/2025.
//

#pragma once

#include <defs.h>
#include <engine/context.h>
#include <engine/domain.h>
#include <input/names.h>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>
#include <array>
#include <containers/ud_map.h>
#include <entt/signal/sigh.hpp>
#include <glm/vec2.hpp>
#include <memory>
#include <string>

#include "SDL3/SDL_vulkan.h"

namespace v {
    class WindowContext;
    class SDLContext;

    class Window : public Domain<Window> {
        friend class WindowContext;

    public:
        Window(Engine& engine, std::string name, glm::ivec2 size, glm::ivec2 pos);
        ~Window();
        // Window event signals

        /// Fires whenever the window is resized
        entt::sink<entt::sigh<void(glm::uvec2)>> on_resize{ sig_resize_ };

        /// Fires whenever the window is closed (user pressed 'X' or
        /// Alt-F4'd)
        entt::sink<entt::sigh<void()>> on_close{ sig_close_ };

        /// Fires whenever focus is gained/lost for a window (the bool
        /// parameter is true if focus was gained, false if lost)
        entt::sink<entt::sigh<void(bool)>> on_focus{ sig_focus_ };

        /// Fires whenever the window is moved
        entt::sink<entt::sigh<void(glm::ivec2)>> on_moved{ sig_moved_ };

        /// Fires whenever the window is minimized
        entt::sink<entt::sigh<void()>> on_minimized{ sig_minimized_ };

        /// Fires whenever the window is maximized
        entt::sink<entt::sigh<void()>> on_maximized{ sig_maximized_ };

        /// Fires whenever the window is restored from minimized/maximized
        entt::sink<entt::sigh<void()>> on_restored{ sig_restored_ };

        /// Fires whenever the mouse cursor enters the window
        entt::sink<entt::sigh<void()>> on_mouse_enter{ sig_mouse_enter_ };

        /// Fires whenever the mouse cursor leaves the window
        entt::sink<entt::sigh<void()>> on_mouse_leave{ sig_mouse_leave_ };

        /// Fires whenever the window enters fullscreen mode
        entt::sink<entt::sigh<void()>> on_fullscreen_enter{ sig_fullscreen_enter_ };

        /// Fires whenever the window leaves fullscreen mode
        entt::sink<entt::sigh<void()>> on_fullscreen_leave{ sig_fullscreen_leave_ };

        /// Fires whenever the window is moved to a different display
        entt::sink<entt::sigh<void()>> on_display_changed{ sig_display_changed_ };

        // Input event signals

        /// Fires whenever a key is pressed (not held)
        entt::sink<entt::sigh<void(Key)>> on_key_pressed{ sig_key_pressed_ };

        /// Fires whenever a key is released
        entt::sink<entt::sigh<void(Key)>> on_key_released{ sig_key_released_ };

        /// Fires whenever a mouse button is pressed (not held)
        entt::sink<entt::sigh<void(MouseButton)>> on_mouse_pressed{ sig_mouse_pressed_ };

        /// Fires whenever a mouse button is released
        entt::sink<entt::sigh<void(MouseButton)>> on_mouse_released{
            sig_mouse_released_
        };

        /// Fires whenever the mouse is moved (position, relative movement)
        entt::sink<entt::sigh<void(glm::ivec2, glm::ivec2)>> on_mouse_moved{
            sig_mouse_moved_
        };

        /// Fires whenever the mouse wheel is scrolled (x, y scroll
        /// amounts)
        entt::sink<entt::sigh<void(glm::ivec2)>> on_mouse_wheel{ sig_mouse_wheel_ };

        /// Fires whenever text is input (for UI text fields)
        entt::sink<entt::sigh<void(std::string)>> on_text_input{ sig_text_input_ };

        /// Fires whenever a file is dropped onto the window
        entt::sink<entt::sigh<void(std::string)>> on_file_dropped{ sig_file_dropped_ };

        // Window input states

        /// Check if key is currently held down
        bool is_key_down(Key key) const;

        /// Check if key was just pressed this frame
        bool is_key_pressed(Key key) const;

        /// Check if key was just released this frame
        bool is_key_released(Key key) const;

        /// Check if mouse button is currently held down
        bool is_mbutton_down(MouseButton button) const;

        /// Check if mouse button was just pressed this frame
        bool is_mbutton_pressed(MouseButton button) const;

        /// Check if mouse button was just released this frame
        bool is_mbutton_released(MouseButton button) const;

        /// Get current mouse position relative to window
        glm::ivec2 get_mouse_position() const;

        // Window property getters

        /// Get window size
        glm::ivec2 size() const;
        /// Get window position
        glm::ivec2 pos() const;
        /// Get window title
        const std::string& title() const;
        /// Get window opacity (0.0 to 1.0)
        float opacity() const;

        // Window state getters

        /// Check if window is in fullscreen mode
        bool is_fullscreen() const;
        /// Check if window is minimized
        bool is_minimized() const;
        /// Check if window is maximized
        bool is_maximized() const;
        /// Check if window is visible
        bool is_visible() const;
        /// Check if window is resizable
        bool is_resizable() const;
        /// Check if window is always on top
        bool is_always_on_top() const;
        /// Check if window has focus
        bool is_focused() const;
        /// Check if window has raw input captured (relative mouse mode)
        bool capturing_raw_input() const;

        // Window property setters

        /// Set window size
        void set_size(glm::ivec2 size);
        /// Set window position
        void set_pos(glm::ivec2 pos);
        /// Set window title
        void set_title(const std::string& title);
        /// Set window opacity (0.0 to 1.0)
        void set_opacity(float opacity);
        /// Set fullscreen mode
        void set_fullscreen(bool fullscreen);
        /// Set if window is resizable
        void set_resizable(bool resizable);
        /// Set if window is always on top
        void set_always_on_top(bool always_on_top);

        // Window actions

        /// Minimize the window
        void minimize();
        /// Maximize the window
        void maximize();
        /// Restore window from minimized/maximized state
        void restore();
        /// Show the window
        void show();
        /// Hide the window
        void hide();
        /// Raise window to front and give it focus
        void raise();
        /// Flash window to get user attention
        void flash();
        /// Enable/disable raw input capture (relative mouse mode)
        void capture_raw_input(bool capture);

        /// Creates a vulkan surface - directly mirrors vulkan API
        FORCEINLINE bool create_vk_surface(
            VkInstance instance, const VkAllocationCallbacks* allocator,
            VkSurfaceKHR* surface) const
        {
            return SDL_Vulkan_CreateSurface(
                this->sdl_window_, instance, allocator, surface);
        }

        /// Get access to the underlying SDL window - needed for native handle extraction
        FORCEINLINE SDL_Window* get_sdl_window() const { return sdl_window_; }

    private:
        /// Handles individual events and fires appropriate handlers
        void process_event(const SDL_Event& event);

        // Event signals
        entt::sigh<void(glm::uvec2)>             sig_resize_;
        entt::sigh<void()>                       sig_close_;
        entt::sigh<void(bool)>                   sig_focus_;
        entt::sigh<void(glm::ivec2)>             sig_moved_;
        entt::sigh<void()>                       sig_minimized_;
        entt::sigh<void()>                       sig_maximized_;
        entt::sigh<void()>                       sig_restored_;
        entt::sigh<void()>                       sig_mouse_enter_;
        entt::sigh<void()>                       sig_mouse_leave_;
        entt::sigh<void()>                       sig_fullscreen_enter_;
        entt::sigh<void()>                       sig_fullscreen_leave_;
        entt::sigh<void()>                       sig_display_changed_;
        entt::sigh<void(Key)>                    sig_key_pressed_;
        entt::sigh<void(Key)>                    sig_key_released_;
        entt::sigh<void(MouseButton)>            sig_mouse_pressed_;
        entt::sigh<void(MouseButton)>            sig_mouse_released_;
        entt::sigh<void(glm::ivec2, glm::ivec2)> sig_mouse_moved_;
        entt::sigh<void(glm::ivec2)>             sig_mouse_wheel_;
        entt::sigh<void(std::string)>            sig_text_input_;
        entt::sigh<void(std::string)>            sig_file_dropped_;

        // Internal stuff
        SDL_Window* sdl_window_;
        glm::ivec2  size_, pos_;
        std::string name_;

        // Input states
        std::array<bool, SDL_SCANCODE_COUNT> curr_keys_;
        std::array<bool, SDL_SCANCODE_COUNT> prev_keys_;
        // SDL supports up to 8 mouse buttons
        std::array<bool, 8> curr_mbuttons;
        std::array<bool, 8> prev_mbuttons;
        glm::ivec2          mouse_pos_;
    };

    /// A context for managing windows and input related to windows.
    /// TODO! there is only one window for now, it is a singleton as it makes it
    /// easy to write a quick prototype of the rendering system.  revisit this if needed
    /// Kind of a special domain, in the sense that the 'components' (windows)
    /// are not tied to the lifetime of a particular domain
    class WindowContext : public Context {
        friend class SDLContext;

    public:
        explicit WindowContext(Engine& engine);
        ~WindowContext();

        /// Create a window with the given parameters
        /// @return A pointer to the window, or nullptr if failure.  Window must be
        /// destroyed with WindowContext::destroy_window.
        /// TODO! this is a no-op if a window already exists, because for now the context
        /// only supports singleton windows
        Window* create_window(const std::string& name, glm::ivec2 size, glm::ivec2 pos);

        /// Gets the Window singleton
        FORCEINLINE Window* get_window() const { return singleton_; }

        /// Destroy a window
        void destroy_window(const Window* window);

        /// Updates windows, pumps events, etc. Should be called at the
        /// desired input update rate, and should be called first in an
        /// application loop to guarantee updated input
        void update();

    private:
        /// Handles events routed from SDLContext for window-specific processing
        /// Called by SDLContext as a friend class
        /// @deprecated and unused for now, this used to be called directly by SDL ctx,
        /// but made a change so each window registers its own SDLComponent
        [[maybe_unused]] void handle_events(const SDL_Event& event);

        ud_map<Uint32, Window*> windows_;

        Window* singleton_{ nullptr };
    };
} // namespace v
