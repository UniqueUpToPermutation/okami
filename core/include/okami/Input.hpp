#pragma once

#include <functional>

#include <okami/PlatformDefs.hpp>
#include <okami/Event.hpp>
#include <glm/vec2.hpp>

namespace okami::core {
    class IInputProvider;

    enum class CallbackPriority {
        LOW = 0,
        MEDIUM = 1,
        HIGH = 2,
        GUI = 3
    };

    class IInputCapture {
    public:
        virtual bool ShouldCaptureMouse() const = 0;
        virtual bool ShouldCaptureKeyboard() const = 0;
    };

    enum class CursorMode {
        NORMAL,
        HIDDEN,
        DISABLED
    };

    enum class KeyState {
        PRESS,
        RELEASE
    };

    enum class MouseButton {
        LEFT = 0,
        RIGHT = 1,
        MIDDLE = 2,
        BUTTON4 = 3,
        BUTTON5 = 4,
        BUTTON6 = 5,
        BUTTON7 = 6,
        BUTTON8 = 7
    };

    enum class Key {
        UNKNOWN,
        SPACE,
        APOSTROPHE,
        COMMA,
        MINUS,
        PERIOD,
        SLASH,
        _0,
        _1,
        _2,
        _3,
        _4,
        _5,
        _6,
        _7,
        _8,
        _9,
        SEMICOLON,
        EQUAL,
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
        LEFT_BRACKET,
        BACKSLASH,
        RIGHT_BRACKET,
        GRAVE_ACCENT,
        WORLD1,
        WORLD2,
        ESCAPE,
        ENTER,
        TAB,
        BACKSPACE,
        INSERT,
        DELETE,
        RIGHT,
        LEFT,
        DOWN,
        UP,
        PAGE_UP,
        PAGE_DOWN,
        HOME,
        END,
        CAPS_LOCK,
        SCROLL_LOCK,
        NUM_LOCK,
        PRINT_SCREEN,
        PAUSE,
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
        F13,
        F14,
        F15,
        F16,
        F17,
        F18,
        F19,
        F20,
        F21,
        F22,
        F23,
        F24,
        F25,
        NUMPAD0,
        NUMPAD1,
        NUMPAD2,
        NUMPAD3,
        NUMPAD4,
        NUMPAD5,
        NUMPAD6,
        NUMPAD7,
        NUMPAD8,
        NUMPAD9,
        NUMPAD_DECIMAL,
        NUMPAD_DIVIDE,
        NUMPAD_MULTIPLY,
        NUMPAD_SUBSTRACT,
        NUMPAD_ADD,
        NUMPAD_ENTER,
        NUMPAD_EQUAL,
        LEFT_SHIFT,
        LEFT_CONTROL,
        LEFT_ALT,
        LEFT_SUPER,
        RIGHT_SHIFT,
        RIGHT_CONTROL,
        RIGHT_ALT,
        RIGHT_SUPER,
        MENU
    };

    typedef uint32_t KeyModifiers;

    struct KeyModifier {
        constexpr static KeyModifiers SHIFT = 0x0001;
        constexpr static KeyModifiers CONTROL = 0x0002;
        constexpr static KeyModifiers ALT = 0x0004;
        constexpr static KeyModifiers SUPER = 0x0008;
        constexpr static KeyModifiers CAPS_LOCK = 0x0010;
        constexpr static KeyModifiers NUM_LOCK = 0x0020;
    };

    enum class KeyAction {
        PRESS,
        REPEAT,
        RELEASE
    };

    typedef std::function<bool(
        IInputProvider* input,
        Key key,
        int scancode,
        KeyAction action,
        KeyModifiers mods)> key_callback_t;

    typedef std::function<bool(
        IInputProvider* input,
        unsigned int codepoint)> char_callback_t;

    typedef std::function<bool(
        IInputProvider* input,
        double xoffset,
        double yoffset)> scroll_callback_t;

    typedef std::function<bool(
        IInputProvider* input,
        MouseButton button,
        KeyAction action,
        KeyModifiers mods)> mouse_button_callback_t;

    typedef std::function<bool(
        IInputProvider* input,
        int entered)> cursor_enter_callback_t;

    typedef std::function<bool(
        IInputProvider* input,
        unsigned int codepoint,
        KeyModifiers mods)> char_mod_callback_t;

    typedef std::function<bool(
        IInputProvider* input,
        int path_count,
        const char* paths[])> drop_callback_t;

    typedef std::function<bool(
        IInputProvider* input,
        double xpos,
        double ypos)> cursor_pos_callback_t;

    class IInputProvider {
    public:
        virtual KeyState GetState(MouseButton mouseButton) const = 0;
        virtual KeyState GetState(Key key) const = 0;
        virtual glm::dvec2 GetCursorPos() const = 0;
        virtual void SetCursorPos(const glm::dvec2& pos) = 0;
        virtual void SetCursorMode(CursorMode cursor) = 0;
        virtual CursorMode GetCursorMode() const = 0;
        virtual void SetRawMouseMotion(bool enabled) = 0;
        virtual bool IsRawMouseMotionSupported() const = 0;

        virtual const char* GetClipboardText() = 0;
        virtual void SetClipboardText(const char* text) = 0;

        virtual core::delegate_handle_t 
            AddMouseButtonCallback(
                const mouse_button_callback_t& callback, 
                CallbackPriority priority = CallbackPriority::MEDIUM,
                IInputCapture* inputCapture = nullptr) = 0;
        virtual core::delegate_handle_t 
            AddKeyCallback(
                const key_callback_t& calback,
                CallbackPriority priority = CallbackPriority::MEDIUM,
                IInputCapture* inputCapture = nullptr) = 0;
        virtual core::delegate_handle_t 
            AddCharCallback(
                const char_callback_t& callback, 
                CallbackPriority priority = CallbackPriority::MEDIUM,
                IInputCapture* inputCapture = nullptr) = 0;
        virtual core::delegate_handle_t 
            AddScrollCallback(
                const scroll_callback_t& callback,
                CallbackPriority priority = CallbackPriority::MEDIUM,
                IInputCapture* inputCapture = nullptr) = 0;
        virtual core::delegate_handle_t
            AddCharModCallback(
                const char_mod_callback_t& callback,
                CallbackPriority priority = CallbackPriority::MEDIUM,
                IInputCapture* inputCapture = nullptr) = 0;
        virtual core::delegate_handle_t
            AddCursorPosCallback(
                const cursor_pos_callback_t& callback,
                CallbackPriority priority = CallbackPriority::MEDIUM,
                IInputCapture* inputCapture = nullptr) = 0;
        virtual core::delegate_handle_t
            AddCursorEnterCallback(
                const cursor_enter_callback_t& callback,
                CallbackPriority priority = CallbackPriority::MEDIUM,
                IInputCapture* inputCapture = nullptr) = 0;
        virtual core::delegate_handle_t
            AddDropCallback(
                const drop_callback_t& callback,
                CallbackPriority priority = CallbackPriority::MEDIUM) = 0;

        virtual void
            RemoveMouseButtonCallback(core::delegate_handle_t) = 0;
        virtual void
            RemoveKeyCallback(core::delegate_handle_t) = 0;
        virtual void
            RemoveCharCallback(core::delegate_handle_t) = 0;
        virtual void
            RemoveScrollCallback(core::delegate_handle_t) = 0;
        virtual void
            RemoveCharModCallback(core::delegate_handle_t) = 0;
        virtual void
            RemoveCursorPosCallback(core::delegate_handle_t) = 0;
        virtual void
            RemoveDropCallback(core::delegate_handle_t) = 0;
        virtual void
            RemoveCursorEnterCallback(core::delegate_handle_t) = 0;

        virtual void* GetBackend() const = 0;
        virtual IInputCapture* GetMouseFocus() = 0;
        virtual IInputCapture* GetKeyboardFocus() = 0;

        virtual void RegisterInterfaces(core::InterfaceCollection& interfaces) = 0;

        virtual void WaitForInput() = 0;

        inline void SetCursorPos(double x, double y) {
            SetCursorPos(glm::dvec2(x, y));
        }
    };
}