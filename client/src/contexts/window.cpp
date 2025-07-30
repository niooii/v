//
// Created by niooi on 7/29/2025.
//

#include <SDL3/SDL.h>
#include <SDL3/SDL_keyboard.h>
#include <contexts/window.h>
#include <ranges>

namespace v {
    // Utility methods

    static SDL_Scancode key_to_sdl(Key key);
    static Key          sdl_to_key(SDL_Scancode scancode);
    static Uint8        mbutton_to_sdl(MouseButton button);
    static MouseButton  sdl_to_mbutton(Uint8 button);
    static bool         has_window_id(Uint32 event_type);

    // Window object methods (public)

    Window::Window(std::string name, glm::ivec2 size, glm::ivec2 pos) :
        sdl_window_(nullptr), size_(size), pos_(pos), name_(std::move(name)),
        curr_keys_{}, prev_keys_{}, curr_mbuttons{}, prev_mbuttons{}, mouse_pos_(0, 0)
    {
        const SDL_PropertiesID props = SDL_CreateProperties();
        SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, name_.c_str());
        SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, pos.x);
        SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, pos.y);
        SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, size.x);
        SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, size.y);
        SDL_SetNumberProperty(
            props, SDL_PROP_WINDOW_CREATE_FLAGS_NUMBER,
            SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);

        sdl_window_ = SDL_CreateWindowWithProperties(props);

        SDL_DestroyProperties(props);

        if (!sdl_window_)
            throw std::runtime_error(SDL_GetError());
    }

    Window::~Window()
    {
        if (sdl_window_)
        {
            if (this->capturing_raw_input())
                this->capture_raw_input(false);
            SDL_DestroyWindow(sdl_window_);
            sdl_window_ = nullptr;
        }
    }

    bool Window::is_key_down(Key key) const
    {
        const SDL_Scancode scancode = key_to_sdl(key);
        return curr_keys_[scancode];
    }

    bool Window::is_key_pressed(Key key) const
    {
        const SDL_Scancode scancode = key_to_sdl(key);
        return curr_keys_[scancode] && !prev_keys_[scancode];
    }

    bool Window::is_key_released(Key key) const
    {
        const SDL_Scancode scancode = key_to_sdl(key);
        return !curr_keys_[scancode] && prev_keys_[scancode];
    }

    bool Window::is_mbutton_down(MouseButton button) const
    {
        const Uint8 sdl_button = mbutton_to_sdl(button);
        if (sdl_button == 0 || sdl_button > 8)
            return false;
        return curr_mbuttons[sdl_button - 1];
    }

    bool Window::is_mbutton_pressed(MouseButton button) const
    {
        const Uint8 sdl_button = mbutton_to_sdl(button);
        if (sdl_button == 0 || sdl_button > 8)
            return false;
        return curr_mbuttons[sdl_button - 1] && !prev_mbuttons[sdl_button - 1];
    }

    bool Window::is_mbutton_released(MouseButton button) const
    {
        const Uint8 sdl_button = mbutton_to_sdl(button);
        if (sdl_button == 0 || sdl_button > 8)
            return false;
        return !curr_mbuttons[sdl_button - 1] && prev_mbuttons[sdl_button - 1];
    }

    glm::ivec2 Window::get_mouse_position() const { return mouse_pos_; }

    // Window property getters

    glm::ivec2 Window::size() const { return size_; }

    glm::ivec2 Window::pos() const { return pos_; }

    const std::string& Window::title() const { return name_; }

    float Window::opacity() const
    {
        float opacity;
        return SDL_GetWindowOpacity(sdl_window_);;
    }

    // Window state getters

    bool Window::is_fullscreen() const
    {
        return SDL_GetWindowFlags(sdl_window_) & SDL_WINDOW_FULLSCREEN;
    }

    bool Window::is_minimized() const
    {
        return SDL_GetWindowFlags(sdl_window_) & SDL_WINDOW_MINIMIZED;
    }

    bool Window::is_maximized() const
    {
        return SDL_GetWindowFlags(sdl_window_) & SDL_WINDOW_MAXIMIZED;
    }

    bool Window::is_visible() const
    {
        return !(SDL_GetWindowFlags(sdl_window_) & SDL_WINDOW_HIDDEN);
    }

    bool Window::is_resizable() const
    {
        return SDL_GetWindowFlags(sdl_window_) & SDL_WINDOW_RESIZABLE;
    }

    bool Window::is_always_on_top() const
    {
        return SDL_GetWindowFlags(sdl_window_) & SDL_WINDOW_ALWAYS_ON_TOP;
    }

    bool Window::is_focused() const
    {
        return SDL_GetWindowFlags(sdl_window_) & SDL_WINDOW_INPUT_FOCUS;
    }

    bool Window::capturing_raw_input() const
    {
        return SDL_GetWindowRelativeMouseMode(sdl_window_);
    }

    // Window property setters

    void Window::set_size(glm::ivec2 size)
    {
        size_ = size;
        SDL_SetWindowSize(sdl_window_, size.x, size.y);
    }

    void Window::set_pos(glm::ivec2 pos)
    {
        pos_ = pos;
        SDL_SetWindowPosition(sdl_window_, pos.x, pos.y);
    }

    void Window::set_title(const std::string& title)
    {
        name_ = title;
        SDL_SetWindowTitle(sdl_window_, title.c_str());
    }

    void Window::set_opacity(float opacity)
    {
        SDL_SetWindowOpacity(sdl_window_, opacity);
    }

    void Window::set_fullscreen(bool fullscreen)
    {
        SDL_SetWindowFullscreen(sdl_window_, fullscreen);
    }

    void Window::set_resizable(bool resizable)
    {
        SDL_SetWindowResizable(sdl_window_, resizable);
    }

    void Window::set_always_on_top(bool always_on_top)
    {
        SDL_SetWindowAlwaysOnTop(sdl_window_, always_on_top);
    }

    // Window actions

    void Window::minimize()
    {
        SDL_MinimizeWindow(sdl_window_);
    }

    void Window::maximize()
    {
        SDL_MaximizeWindow(sdl_window_);
    }

    void Window::restore()
    {
        SDL_RestoreWindow(sdl_window_);
    }

    void Window::show()
    {
        SDL_ShowWindow(sdl_window_);
    }

    void Window::hide()
    {
        SDL_HideWindow(sdl_window_);
    }

    void Window::raise()
    {
        SDL_RaiseWindow(sdl_window_);
    }

    void Window::flash()
    {
        SDL_FlashWindow(sdl_window_, SDL_FLASH_BRIEFLY);
    }

    void Window::capture_raw_input(bool capture)
    {
        SDL_SetWindowRelativeMouseMode(sdl_window_, capture);
    }

    // Window object methods (private)

    void Window::process_event(const SDL_Event& event)
    {
        switch (event.type)
        {
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            sig_close_.publish();
            break;

        case SDL_EVENT_WINDOW_RESIZED:
            size_ = { event.window.data1, event.window.data2 };
            sig_resize_.publish(size_);
            break;

        case SDL_EVENT_WINDOW_MOVED:
            pos_ = { event.window.data1, event.window.data2 };
            sig_moved_.publish(pos_);
            break;

        case SDL_EVENT_WINDOW_FOCUS_GAINED:
        case SDL_EVENT_WINDOW_FOCUS_LOST:
            sig_focus_.publish(event.type == SDL_EVENT_WINDOW_FOCUS_GAINED);
            break;

        case SDL_EVENT_WINDOW_MINIMIZED:
            sig_minimized_.publish();
            break;

        case SDL_EVENT_WINDOW_MAXIMIZED:
            sig_maximized_.publish();
            break;

        case SDL_EVENT_WINDOW_RESTORED:
            sig_restored_.publish();
            break;

        case SDL_EVENT_WINDOW_MOUSE_ENTER:
            sig_mouse_enter_.publish();
            break;

        case SDL_EVENT_WINDOW_MOUSE_LEAVE:
            sig_mouse_leave_.publish();
            break;

        case SDL_EVENT_WINDOW_ENTER_FULLSCREEN:
            sig_fullscreen_enter_.publish();
            break;

        case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN:
            sig_fullscreen_leave_.publish();
            break;

        case SDL_EVENT_WINDOW_DISPLAY_CHANGED:
            sig_display_changed_.publish();
            break;

        case SDL_EVENT_KEY_DOWN:
            curr_keys_[event.key.scancode] = true;
            sig_key_pressed_.publish(sdl_to_key(event.key.scancode));
            break;

        case SDL_EVENT_KEY_UP:
            curr_keys_[event.key.scancode] = false;
            sig_key_released_.publish(sdl_to_key(event.key.scancode));
            break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            curr_mbuttons[event.button.button - 1] = true;
            sig_mouse_pressed_.publish(sdl_to_mbutton(event.button.button));
            break;

        case SDL_EVENT_MOUSE_BUTTON_UP:
            curr_mbuttons[event.button.button - 1] = false;
            sig_mouse_released_.publish(sdl_to_mbutton(event.button.button));
            break;

        case SDL_EVENT_MOUSE_MOTION:
            mouse_pos_ = glm::ivec2(
                static_cast<int>(event.motion.x), static_cast<int>(event.motion.y));
            sig_mouse_moved_.publish(
                mouse_pos_,
                glm::ivec2(
                    static_cast<int>(event.motion.xrel),
                    static_cast<int>(event.motion.yrel)));
            break;

        case SDL_EVENT_MOUSE_WHEEL:
            sig_mouse_wheel_.publish(
                glm::ivec2(
                    static_cast<int>(event.wheel.x), static_cast<int>(event.wheel.y)));
            break;

        case SDL_EVENT_TEXT_INPUT:
            sig_text_input_.publish(std::string(event.text.text));
            break;

        case SDL_EVENT_DROP_FILE:
            sig_file_dropped_.publish(std::string(event.drop.data));
            break;

        default:
            // Ignore other events for now
            break;
        }
    }

    // Window context manager methods

    WindowContext::WindowContext()
    {
        SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    }

    WindowContext::~WindowContext()
    {
        SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    }

    Window*
    WindowContext::create_window(const std::string& name, glm::ivec2 size, glm::ivec2 pos)
    {
        try
        {
            const auto window = new Window(name, size, pos);

            if (const auto id = SDL_GetWindowID(window->sdl_window_))
            {
                windows_[id] = window;
                return window;
            }
        }
        catch (std::runtime_error& e)
        {
            LOG_ERROR(e.what());
        }

        LOG_ERROR("Failed to create window with name {}", name);

        return nullptr;
    }

    void WindowContext::destroy_window(const Window* window)
    {
        if (!window)
            return;

        const auto id = SDL_GetWindowID(window->sdl_window_);
        LOG_DEBUG("Destroying window with id {} and addr {}", (u64)id, (u64)windows_[id]);
        windows_.erase(id);

        delete window;
    }

    void WindowContext::update()
    {
        SDL_Event event;

        // update the
        for (const auto& w : windows_ | std::views::values)
        {
            // Copy current state to prev state
            w->prev_keys_    = w->curr_keys_;
            w->prev_mbuttons = w->curr_mbuttons;
        }

        // send events to their respective windows
        while (SDL_PollEvent(&event))
        {
            if (has_window_id(event.type))
            {
                const auto id = event.window.windowID;
                if (!windows_.contains(id))
                    continue;

                windows_[id]->process_event(event);

                // we process destruction of window here in case
                // we have any listeners that should be notified (from
                // process_event)
                if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
                    this->destroy_window(windows_[id]);

                continue;
            }
            // TODO! global non window related events, process them
            // somehow
        }
    }

    // Utility function impls

    static MouseButton sdl_to_mbutton(const Uint8 button)
    {
        switch (button)
        {
        case SDL_BUTTON_LEFT:
            return MouseButton::Left;
        case SDL_BUTTON_RIGHT:
            return MouseButton::Right;
        case SDL_BUTTON_MIDDLE:
            return MouseButton::Middle;
        case SDL_BUTTON_X1:
            return MouseButton::X1;
        case SDL_BUTTON_X2:
            return MouseButton::X2;
        default:
            return MouseButton::Unknown;
        }
    }

    static Uint8 mbutton_to_sdl(MouseButton button)
    {
        switch (button)
        {
        case MouseButton::Left:
            return SDL_BUTTON_LEFT;
        case MouseButton::Right:
            return SDL_BUTTON_RIGHT;
        case MouseButton::Middle:
            return SDL_BUTTON_MIDDLE;
        case MouseButton::X1:
            return SDL_BUTTON_X1;
        case MouseButton::X2:
            return SDL_BUTTON_X2;
        default:
        case MouseButton::Unknown:
            return 0;
        }
    }

    static SDL_Scancode key_to_sdl(const Key key)
    {
        switch (key)
        {
        // Letters
        case Key::A:
            return SDL_SCANCODE_A;
        case Key::B:
            return SDL_SCANCODE_B;
        case Key::C:
            return SDL_SCANCODE_C;
        case Key::D:
            return SDL_SCANCODE_D;
        case Key::E:
            return SDL_SCANCODE_E;
        case Key::F:
            return SDL_SCANCODE_F;
        case Key::G:
            return SDL_SCANCODE_G;
        case Key::H:
            return SDL_SCANCODE_H;
        case Key::I:
            return SDL_SCANCODE_I;
        case Key::J:
            return SDL_SCANCODE_J;
        case Key::K:
            return SDL_SCANCODE_K;
        case Key::L:
            return SDL_SCANCODE_L;
        case Key::M:
            return SDL_SCANCODE_M;
        case Key::N:
            return SDL_SCANCODE_N;
        case Key::O:
            return SDL_SCANCODE_O;
        case Key::P:
            return SDL_SCANCODE_P;
        case Key::Q:
            return SDL_SCANCODE_Q;
        case Key::R:
            return SDL_SCANCODE_R;
        case Key::S:
            return SDL_SCANCODE_S;
        case Key::T:
            return SDL_SCANCODE_T;
        case Key::U:
            return SDL_SCANCODE_U;
        case Key::V:
            return SDL_SCANCODE_V;
        case Key::W:
            return SDL_SCANCODE_W;
        case Key::X:
            return SDL_SCANCODE_X;
        case Key::Y:
            return SDL_SCANCODE_Y;
        case Key::Z:
            return SDL_SCANCODE_Z;

        // Numbers
        case Key::Num0:
            return SDL_SCANCODE_0;
        case Key::Num1:
            return SDL_SCANCODE_1;
        case Key::Num2:
            return SDL_SCANCODE_2;
        case Key::Num3:
            return SDL_SCANCODE_3;
        case Key::Num4:
            return SDL_SCANCODE_4;
        case Key::Num5:
            return SDL_SCANCODE_5;
        case Key::Num6:
            return SDL_SCANCODE_6;
        case Key::Num7:
            return SDL_SCANCODE_7;
        case Key::Num8:
            return SDL_SCANCODE_8;
        case Key::Num9:
            return SDL_SCANCODE_9;

        // Function keys
        case Key::F1:
            return SDL_SCANCODE_F1;
        case Key::F2:
            return SDL_SCANCODE_F2;
        case Key::F3:
            return SDL_SCANCODE_F3;
        case Key::F4:
            return SDL_SCANCODE_F4;
        case Key::F5:
            return SDL_SCANCODE_F5;
        case Key::F6:
            return SDL_SCANCODE_F6;
        case Key::F7:
            return SDL_SCANCODE_F7;
        case Key::F8:
            return SDL_SCANCODE_F8;
        case Key::F9:
            return SDL_SCANCODE_F9;
        case Key::F10:
            return SDL_SCANCODE_F10;
        case Key::F11:
            return SDL_SCANCODE_F11;
        case Key::F12:
            return SDL_SCANCODE_F12;

        // Arrow keys
        case Key::Up:
            return SDL_SCANCODE_UP;
        case Key::Down:
            return SDL_SCANCODE_DOWN;
        case Key::Left:
            return SDL_SCANCODE_LEFT;
        case Key::Right:
            return SDL_SCANCODE_RIGHT;

        // Special keys
        case Key::Space:
            return SDL_SCANCODE_SPACE;
        case Key::Enter:
            return SDL_SCANCODE_RETURN;
        case Key::Escape:
            return SDL_SCANCODE_ESCAPE;
        case Key::Tab:
            return SDL_SCANCODE_TAB;
        case Key::Backspace:
            return SDL_SCANCODE_BACKSPACE;
        case Key::Delete:
            return SDL_SCANCODE_DELETE;
        case Key::Insert:
            return SDL_SCANCODE_INSERT;
        case Key::Home:
            return SDL_SCANCODE_HOME;
        case Key::End:
            return SDL_SCANCODE_END;
        case Key::PageUp:
            return SDL_SCANCODE_PAGEUP;
        case Key::PageDown:
            return SDL_SCANCODE_PAGEDOWN;

        // Modifier keys
        case Key::LeftShift:
            return SDL_SCANCODE_LSHIFT;
        case Key::RightShift:
            return SDL_SCANCODE_RSHIFT;
        case Key::LeftCtrl:
            return SDL_SCANCODE_LCTRL;
        case Key::RightCtrl:
            return SDL_SCANCODE_RCTRL;
        case Key::LeftAlt:
            return SDL_SCANCODE_LALT;
        case Key::RightAlt:
            return SDL_SCANCODE_RALT;

        // Keypad
        case Key::KP0:
            return SDL_SCANCODE_KP_0;
        case Key::KP1:
            return SDL_SCANCODE_KP_1;
        case Key::KP2:
            return SDL_SCANCODE_KP_2;
        case Key::KP3:
            return SDL_SCANCODE_KP_3;
        case Key::KP4:
            return SDL_SCANCODE_KP_4;
        case Key::KP5:
            return SDL_SCANCODE_KP_5;
        case Key::KP6:
            return SDL_SCANCODE_KP_6;
        case Key::KP7:
            return SDL_SCANCODE_KP_7;
        case Key::KP8:
            return SDL_SCANCODE_KP_8;
        case Key::KP9:
            return SDL_SCANCODE_KP_9;
        case Key::KPPlus:
            return SDL_SCANCODE_KP_PLUS;
        case Key::KPMinus:
            return SDL_SCANCODE_KP_MINUS;
        case Key::KPMultiply:
            return SDL_SCANCODE_KP_MULTIPLY;
        case Key::KPDivide:
            return SDL_SCANCODE_KP_DIVIDE;
        case Key::KPEnter:
            return SDL_SCANCODE_KP_ENTER;
        case Key::KPPeriod:
            return SDL_SCANCODE_KP_PERIOD;

        default:
        case Key::Unknown:
            return SDL_SCANCODE_UNKNOWN;
        }
    }

    static Key sdl_to_key(const SDL_Scancode scancode)
    {
        switch (scancode)
        {
        // Letters
        case SDL_SCANCODE_A:
            return Key::A;
        case SDL_SCANCODE_B:
            return Key::B;
        case SDL_SCANCODE_C:
            return Key::C;
        case SDL_SCANCODE_D:
            return Key::D;
        case SDL_SCANCODE_E:
            return Key::E;
        case SDL_SCANCODE_F:
            return Key::F;
        case SDL_SCANCODE_G:
            return Key::G;
        case SDL_SCANCODE_H:
            return Key::H;
        case SDL_SCANCODE_I:
            return Key::I;
        case SDL_SCANCODE_J:
            return Key::J;
        case SDL_SCANCODE_K:
            return Key::K;
        case SDL_SCANCODE_L:
            return Key::L;
        case SDL_SCANCODE_M:
            return Key::M;
        case SDL_SCANCODE_N:
            return Key::N;
        case SDL_SCANCODE_O:
            return Key::O;
        case SDL_SCANCODE_P:
            return Key::P;
        case SDL_SCANCODE_Q:
            return Key::Q;
        case SDL_SCANCODE_R:
            return Key::R;
        case SDL_SCANCODE_S:
            return Key::S;
        case SDL_SCANCODE_T:
            return Key::T;
        case SDL_SCANCODE_U:
            return Key::U;
        case SDL_SCANCODE_V:
            return Key::V;
        case SDL_SCANCODE_W:
            return Key::W;
        case SDL_SCANCODE_X:
            return Key::X;
        case SDL_SCANCODE_Y:
            return Key::Y;
        case SDL_SCANCODE_Z:
            return Key::Z;

        // Numbers
        case SDL_SCANCODE_0:
            return Key::Num0;
        case SDL_SCANCODE_1:
            return Key::Num1;
        case SDL_SCANCODE_2:
            return Key::Num2;
        case SDL_SCANCODE_3:
            return Key::Num3;
        case SDL_SCANCODE_4:
            return Key::Num4;
        case SDL_SCANCODE_5:
            return Key::Num5;
        case SDL_SCANCODE_6:
            return Key::Num6;
        case SDL_SCANCODE_7:
            return Key::Num7;
        case SDL_SCANCODE_8:
            return Key::Num8;
        case SDL_SCANCODE_9:
            return Key::Num9;

        // Function keys
        case SDL_SCANCODE_F1:
            return Key::F1;
        case SDL_SCANCODE_F2:
            return Key::F2;
        case SDL_SCANCODE_F3:
            return Key::F3;
        case SDL_SCANCODE_F4:
            return Key::F4;
        case SDL_SCANCODE_F5:
            return Key::F5;
        case SDL_SCANCODE_F6:
            return Key::F6;
        case SDL_SCANCODE_F7:
            return Key::F7;
        case SDL_SCANCODE_F8:
            return Key::F8;
        case SDL_SCANCODE_F9:
            return Key::F9;
        case SDL_SCANCODE_F10:
            return Key::F10;
        case SDL_SCANCODE_F11:
            return Key::F11;
        case SDL_SCANCODE_F12:
            return Key::F12;

        // Arrow keys
        case SDL_SCANCODE_UP:
            return Key::Up;
        case SDL_SCANCODE_DOWN:
            return Key::Down;
        case SDL_SCANCODE_LEFT:
            return Key::Left;
        case SDL_SCANCODE_RIGHT:
            return Key::Right;

        // Special keys
        case SDL_SCANCODE_SPACE:
            return Key::Space;
        case SDL_SCANCODE_RETURN:
            return Key::Enter;
        case SDL_SCANCODE_ESCAPE:
            return Key::Escape;
        case SDL_SCANCODE_TAB:
            return Key::Tab;
        case SDL_SCANCODE_BACKSPACE:
            return Key::Backspace;
        case SDL_SCANCODE_DELETE:
            return Key::Delete;
        case SDL_SCANCODE_INSERT:
            return Key::Insert;
        case SDL_SCANCODE_HOME:
            return Key::Home;
        case SDL_SCANCODE_END:
            return Key::End;
        case SDL_SCANCODE_PAGEUP:
            return Key::PageUp;
        case SDL_SCANCODE_PAGEDOWN:
            return Key::PageDown;

        // Modifier keys
        case SDL_SCANCODE_LSHIFT:
            return Key::LeftShift;
        case SDL_SCANCODE_RSHIFT:
            return Key::RightShift;
        case SDL_SCANCODE_LCTRL:
            return Key::LeftCtrl;
        case SDL_SCANCODE_RCTRL:
            return Key::RightCtrl;
        case SDL_SCANCODE_LALT:
            return Key::LeftAlt;
        case SDL_SCANCODE_RALT:
            return Key::RightAlt;

        // Keypad
        case SDL_SCANCODE_KP_0:
            return Key::KP0;
        case SDL_SCANCODE_KP_1:
            return Key::KP1;
        case SDL_SCANCODE_KP_2:
            return Key::KP2;
        case SDL_SCANCODE_KP_3:
            return Key::KP3;
        case SDL_SCANCODE_KP_4:
            return Key::KP4;
        case SDL_SCANCODE_KP_5:
            return Key::KP5;
        case SDL_SCANCODE_KP_6:
            return Key::KP6;
        case SDL_SCANCODE_KP_7:
            return Key::KP7;
        case SDL_SCANCODE_KP_8:
            return Key::KP8;
        case SDL_SCANCODE_KP_9:
            return Key::KP9;
        case SDL_SCANCODE_KP_PLUS:
            return Key::KPPlus;
        case SDL_SCANCODE_KP_MINUS:
            return Key::KPMinus;
        case SDL_SCANCODE_KP_MULTIPLY:
            return Key::KPMultiply;
        case SDL_SCANCODE_KP_DIVIDE:
            return Key::KPDivide;
        case SDL_SCANCODE_KP_ENTER:
            return Key::KPEnter;
        case SDL_SCANCODE_KP_PERIOD:
            return Key::KPPeriod;

        default:
            return Key::Unknown;
        }
    }

    static bool has_window_id(const Uint32 event_type)
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
