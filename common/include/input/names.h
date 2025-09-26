//
// Created by niooi on 7/29/2025.
//

#pragma once

namespace v {
    /// Keyboard keys
    enum class Key {
        // Letters
        A,
        B,
        C,
        D,
        E,
        F,
        G,
        H,
        I,
        J,
        K,
        L,
        M,
        N,
        O,
        P,
        Q,
        R,
        S,
        T,
        U,
        V,
        W,
        X,
        Y,
        Z,

        // Numbers
        Num0,
        Num1,
        Num2,
        Num3,
        Num4,
        Num5,
        Num6,
        Num7,
        Num8,
        Num9,

        // Function keys
        F1,
        F2,
        F3,
        F4,
        F5,
        F6,
        F7,
        F8,
        F9,
        F10,
        F11,
        F12,

        // Arrow keys
        Up,
        Down,
        Left,
        Right,

        // Special keys
        Space,
        Enter,
        Escape,
        Tab,
        Backspace,
        Delete,
        Insert,
        Home,
        End,
        PageUp,
        PageDown,

        // Modifier keys
        LeftShift,
        RightShift,
        LeftCtrl,
        RightCtrl,
        LeftAlt,
        RightAlt,

        // Keypad
        KP0,
        KP1,
        KP2,
        KP3,
        KP4,
        KP5,
        KP6,
        KP7,
        KP8,
        KP9,
        KPPlus,
        KPMinus,
        KPMultiply,
        KPDivide,
        KPEnter,
        KPPeriod,

        Unknown
    };

    /// Mouse buttons
    enum class MouseButton { Left, Right, Middle, X1, X2, Unknown };
} // namespace v
