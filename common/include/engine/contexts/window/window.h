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
#include <functional>
#include <glm/vec2.hpp>
#include <memory>
#include <string>

#include "SDL3/SDL_vulkan.h"

namespace v {
    class WindowContext;
    class SDLContext;

    /// Component for window-related events (resize, close, focus, etc.)
    struct WindowComponent {
        WindowComponent() = default;

        /// Called when the window is resized (new size)
        std::function<void(glm::uvec2)> on_resize;

        /// Called when the window is closed (user pressed 'X' or Alt-F4'd)
        std::function<void()> on_close;

        /// Called when focus is gained/lost (true if gained, false if lost)
        std::function<void(bool)> on_focus;

        /// Called when the window is moved (new position)
        std::function<void(glm::ivec2)> on_moved;

        /// Called when the window is minimized
        std::function<void()> on_minimized;

        /// Called when the window is maximized
        std::function<void()> on_maximized;

        /// Called when the window is restored from minimized/maximized
        std::function<void()> on_restored;

        /// Called when the window enters fullscreen mode
        std::function<void()> on_fullscreen_enter;

        /// Called when the window leaves fullscreen mode
        std::function<void()> on_fullscreen_leave;

        /// Called when the window is moved to a different display
        std::function<void()> on_display_changed;

        /// Called when a file is dropped onto the window (file path)
        std::function<void(std::string)> on_file_dropped;
    };

    /// Component for mouse-related events
    struct MouseComponent {
        MouseComponent() = default;

        /// Called when a mouse button is pressed (not held)
        std::function<void(MouseButton)> on_mouse_pressed;

        /// Called when a mouse button is released
        std::function<void(MouseButton)> on_mouse_released;

        /// Called when the mouse is moved (position, relative movement)
        std::function<void(glm::ivec2, glm::ivec2)> on_mouse_moved;

        /// Called when the mouse wheel is scrolled (x, y scroll amounts)
        std::function<void(glm::ivec2)> on_mouse_wheel;

        /// Called when the mouse cursor enters the window
        std::function<void()> on_mouse_enter;

        /// Called when the mouse cursor leaves the window
        std::function<void()> on_mouse_leave;
    };

    /// Component for keyboard-related events
    struct KeyComponent {
        KeyComponent() = default;

        /// Called when a key is pressed (not held)
        std::function<void(Key)> on_key_pressed;

        /// Called when a key is released
        std::function<void(Key)> on_key_released;

        /// Called when text is input (for UI text fields)
        std::function<void(std::string)> on_text_input;
    };

    class Window : public Domain<Window> {
        friend class WindowContext;

    public:
        Window(std::string name, glm::ivec2 size, glm::ivec2 pos);
        ~Window();

        void init() override;

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

        /// Get mouse delta (relative movement) from last frame
        /// When capture_raw_input(true) is enabled, this returns relative mouse motion
        glm::ivec2 get_mouse_delta() const;

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
        /// Check if window has mouse captured (relative mouse mode)
        bool capturing_mouse() const;

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
        /// Enable/disable mouse capture (relative mouse mode - hides cursor, locks to
        /// window)
        void capture_mouse(bool capture);

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
        glm::ivec2          mouse_delta_;
    };

    /// A context for managing windows and input related to windows.
    /// TODO! there is only one window for now, it is a singleton as it makes it
    /// easy to write a quick prototype of the rendering system.  revisit this if needed
    /// Kind of a special domain, in the sense that the 'components' (windows)
    /// are not tied to the lifetime of a particular domain
    class WindowContext : public Context<WindowContext> {
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

        /// Create a WindowComponent that will be attached to the given entity
        WindowComponent& create_window_component(entt::entity id);

        /// Create a MouseComponent that will be attached to the given entity
        MouseComponent& create_mouse_component(entt::entity id);

        /// Create a KeyComponent that will be attached to the given entity
        KeyComponent& create_key_component(entt::entity id);

        /// Updates windows, pumps events, etc. Should be called at the
        /// desired input update rate, and should be called first in an
        /// application loop, and right before the input provider's update method (e.g.
        /// SDLContext) to guarantee updated input
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
