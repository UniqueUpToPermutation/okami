#include <okami/diligent/Glfw.hpp>

#if USE_GLFW

#if PLATFORM_MACOS
extern void* GetNSWindowView(GLFWwindow* wnd);
#endif

#include <GLFW/glfw3native.h>

#include <iostream>

void HandleGLFWError(int code, const char* msg) {
    std::cerr << "ERROR (GLFW code " << code << "): " << msg << std::endl;
}

using namespace okami::core;

#define CONV_CASE(x, y) case x: return y
#define CONV_CASE_INV(x, y) case y: return x;

namespace okami::graphics::diligent {

    Key KeyToOkami(int key) {
        switch (key) {
            CONV_CASE(GLFW_KEY_UNKNOWN, Key::UNKNOWN);
            CONV_CASE(GLFW_KEY_SPACE, Key::SPACE);
            CONV_CASE(GLFW_KEY_APOSTROPHE, Key::APOSTROPHE);
            CONV_CASE(GLFW_KEY_COMMA, Key::COMMA);
            CONV_CASE(GLFW_KEY_MINUS, Key::MINUS);
            CONV_CASE(GLFW_KEY_PERIOD, Key::PERIOD);
            CONV_CASE(GLFW_KEY_SLASH, Key::SLASH);
            CONV_CASE(GLFW_KEY_0, Key::_0);
            CONV_CASE(GLFW_KEY_1, Key::_1);
            CONV_CASE(GLFW_KEY_2, Key::_2);
            CONV_CASE(GLFW_KEY_3, Key::_3);
            CONV_CASE(GLFW_KEY_4, Key::_4);
            CONV_CASE(GLFW_KEY_5, Key::_5);
            CONV_CASE(GLFW_KEY_6, Key::_6);
            CONV_CASE(GLFW_KEY_7, Key::_7);
            CONV_CASE(GLFW_KEY_8, Key::_8);
            CONV_CASE(GLFW_KEY_9, Key::_9);
            CONV_CASE(GLFW_KEY_SEMICOLON, Key::SEMICOLON);
            CONV_CASE(GLFW_KEY_EQUAL, Key::EQUAL);
            CONV_CASE(GLFW_KEY_A, Key::A);
            CONV_CASE(GLFW_KEY_B, Key::B);
            CONV_CASE(GLFW_KEY_C, Key::C);
            CONV_CASE(GLFW_KEY_D, Key::D);
            CONV_CASE(GLFW_KEY_E, Key::E);
            CONV_CASE(GLFW_KEY_F, Key::F);
            CONV_CASE(GLFW_KEY_G, Key::G);
            CONV_CASE(GLFW_KEY_H, Key::H);
            CONV_CASE(GLFW_KEY_I, Key::I);
            CONV_CASE(GLFW_KEY_J, Key::J);
            CONV_CASE(GLFW_KEY_K, Key::K);
            CONV_CASE(GLFW_KEY_L, Key::L);
            CONV_CASE(GLFW_KEY_M, Key::M);
            CONV_CASE(GLFW_KEY_N, Key::N);
            CONV_CASE(GLFW_KEY_O, Key::O);
            CONV_CASE(GLFW_KEY_P, Key::P);
            CONV_CASE(GLFW_KEY_Q, Key::Q);
            CONV_CASE(GLFW_KEY_R, Key::R);
            CONV_CASE(GLFW_KEY_S, Key::S);
            CONV_CASE(GLFW_KEY_T, Key::T);
            CONV_CASE(GLFW_KEY_U, Key::U);
            CONV_CASE(GLFW_KEY_V, Key::V);
            CONV_CASE(GLFW_KEY_W, Key::W);
            CONV_CASE(GLFW_KEY_X, Key::X);
            CONV_CASE(GLFW_KEY_Y, Key::Y);
            CONV_CASE(GLFW_KEY_Z, Key::Z);
            CONV_CASE(GLFW_KEY_LEFT_BRACKET, Key::LEFT_BRACKET);
            CONV_CASE(GLFW_KEY_BACKSLASH, Key::BACKSLASH);
            CONV_CASE(GLFW_KEY_RIGHT_BRACKET, Key::RIGHT_BRACKET);
            CONV_CASE(GLFW_KEY_GRAVE_ACCENT, Key::GRAVE_ACCENT);
            CONV_CASE(GLFW_KEY_WORLD_1, Key::WORLD1);
            CONV_CASE(GLFW_KEY_WORLD_2, Key::WORLD2);
            CONV_CASE(GLFW_KEY_ESCAPE, Key::ESCAPE);
            CONV_CASE(GLFW_KEY_ENTER, Key::ENTER);
            CONV_CASE(GLFW_KEY_TAB, Key::TAB);
            CONV_CASE(GLFW_KEY_BACKSPACE, Key::BACKSPACE);
            CONV_CASE(GLFW_KEY_INSERT, Key::INSERT);
            CONV_CASE(GLFW_KEY_DELETE, Key::DELETE);
            CONV_CASE(GLFW_KEY_RIGHT, Key::RIGHT);
            CONV_CASE(GLFW_KEY_LEFT, Key::LEFT);
            CONV_CASE(GLFW_KEY_DOWN, Key::DOWN);
            CONV_CASE(GLFW_KEY_UP, Key::UP);
            CONV_CASE(GLFW_KEY_PAGE_UP, Key::PAGE_UP);
            CONV_CASE(GLFW_KEY_PAGE_DOWN, Key::PAGE_DOWN);
            CONV_CASE(GLFW_KEY_HOME, Key::HOME);
            CONV_CASE(GLFW_KEY_END, Key::END);
            CONV_CASE(GLFW_KEY_CAPS_LOCK, Key::CAPS_LOCK);
            CONV_CASE(GLFW_KEY_SCROLL_LOCK, Key::SCROLL_LOCK);
            CONV_CASE(GLFW_KEY_NUM_LOCK, Key::NUM_LOCK);
            CONV_CASE(GLFW_KEY_PRINT_SCREEN, Key::PRINT_SCREEN);
            CONV_CASE(GLFW_KEY_PAUSE, Key::PAUSE);
            CONV_CASE(GLFW_KEY_F1, Key::F1);
            CONV_CASE(GLFW_KEY_F2, Key::F2);
            CONV_CASE(GLFW_KEY_F3, Key::F3);
            CONV_CASE(GLFW_KEY_F4, Key::F4);
            CONV_CASE(GLFW_KEY_F5, Key::F5);
            CONV_CASE(GLFW_KEY_F6, Key::F6);
            CONV_CASE(GLFW_KEY_F7, Key::F7);
            CONV_CASE(GLFW_KEY_F8, Key::F8);
            CONV_CASE(GLFW_KEY_F9, Key::F9);
            CONV_CASE(GLFW_KEY_F10, Key::F10);
            CONV_CASE(GLFW_KEY_F11, Key::F11);
            CONV_CASE(GLFW_KEY_F12, Key::F12);
            CONV_CASE(GLFW_KEY_F13, Key::F13);
            CONV_CASE(GLFW_KEY_F14, Key::F14);
            CONV_CASE(GLFW_KEY_F15, Key::F15);
            CONV_CASE(GLFW_KEY_F16, Key::F16);
            CONV_CASE(GLFW_KEY_F17, Key::F17);
            CONV_CASE(GLFW_KEY_F18, Key::F18);
            CONV_CASE(GLFW_KEY_F19, Key::F19);
            CONV_CASE(GLFW_KEY_F20, Key::F20);
            CONV_CASE(GLFW_KEY_F21, Key::F21);
            CONV_CASE(GLFW_KEY_F22, Key::F22);
            CONV_CASE(GLFW_KEY_F23, Key::F23);
            CONV_CASE(GLFW_KEY_F24, Key::F24);
            CONV_CASE(GLFW_KEY_F25, Key::F25);
            CONV_CASE(GLFW_KEY_KP_0, Key::NUMPAD0);
            CONV_CASE(GLFW_KEY_KP_1, Key::NUMPAD1);
            CONV_CASE(GLFW_KEY_KP_2, Key::NUMPAD2);
            CONV_CASE(GLFW_KEY_KP_3, Key::NUMPAD3);
            CONV_CASE(GLFW_KEY_KP_4, Key::NUMPAD4);
            CONV_CASE(GLFW_KEY_KP_5, Key::NUMPAD5);
            CONV_CASE(GLFW_KEY_KP_6, Key::NUMPAD6);
            CONV_CASE(GLFW_KEY_KP_7, Key::NUMPAD7);
            CONV_CASE(GLFW_KEY_KP_8, Key::NUMPAD8);
            CONV_CASE(GLFW_KEY_KP_9, Key::NUMPAD9);
            CONV_CASE(GLFW_KEY_KP_DECIMAL, Key::NUMPAD_DECIMAL);
            CONV_CASE(GLFW_KEY_KP_DIVIDE, Key::NUMPAD_DIVIDE);
            CONV_CASE(GLFW_KEY_KP_MULTIPLY, Key::NUMPAD_MULTIPLY);
            CONV_CASE(GLFW_KEY_KP_SUBTRACT, Key::NUMPAD_SUBSTRACT);
            CONV_CASE(GLFW_KEY_KP_ADD, Key::NUMPAD_ADD);
            CONV_CASE(GLFW_KEY_KP_ENTER, Key::NUMPAD_ENTER);
            CONV_CASE(GLFW_KEY_KP_EQUAL, Key::NUMPAD_EQUAL);
            CONV_CASE(GLFW_KEY_LEFT_SHIFT, Key::LEFT_SHIFT);
            CONV_CASE(GLFW_KEY_LEFT_CONTROL, Key::LEFT_CONTROL);
            CONV_CASE(GLFW_KEY_LEFT_ALT, Key::LEFT_ALT);
            CONV_CASE(GLFW_KEY_LEFT_SUPER, Key::LEFT_SUPER);
            CONV_CASE(GLFW_KEY_RIGHT_SHIFT, Key::RIGHT_SHIFT);
            CONV_CASE(GLFW_KEY_RIGHT_CONTROL, Key::RIGHT_CONTROL);
            CONV_CASE(GLFW_KEY_RIGHT_ALT, Key::RIGHT_ALT);
            CONV_CASE(GLFW_KEY_RIGHT_SUPER, Key::RIGHT_SUPER);
            CONV_CASE(GLFW_KEY_MENU, Key::MENU);
        default:
            throw std::runtime_error("Unrecognized!");
        }
    }

    int KeyToGLFW(Key key) {
        switch (key) {
            CONV_CASE_INV(GLFW_KEY_UNKNOWN, Key::UNKNOWN);
            CONV_CASE_INV(GLFW_KEY_SPACE, Key::SPACE);
            CONV_CASE_INV(GLFW_KEY_APOSTROPHE, Key::APOSTROPHE);
            CONV_CASE_INV(GLFW_KEY_COMMA, Key::COMMA);
            CONV_CASE_INV(GLFW_KEY_MINUS, Key::MINUS);
            CONV_CASE_INV(GLFW_KEY_PERIOD, Key::PERIOD);
            CONV_CASE_INV(GLFW_KEY_SLASH, Key::SLASH);
            CONV_CASE_INV(GLFW_KEY_0, Key::_0);
            CONV_CASE_INV(GLFW_KEY_1, Key::_1);
            CONV_CASE_INV(GLFW_KEY_2, Key::_2);
            CONV_CASE_INV(GLFW_KEY_3, Key::_3);
            CONV_CASE_INV(GLFW_KEY_4, Key::_4);
            CONV_CASE_INV(GLFW_KEY_5, Key::_5);
            CONV_CASE_INV(GLFW_KEY_6, Key::_6);
            CONV_CASE_INV(GLFW_KEY_7, Key::_7);
            CONV_CASE_INV(GLFW_KEY_8, Key::_8);
            CONV_CASE_INV(GLFW_KEY_9, Key::_9);
            CONV_CASE_INV(GLFW_KEY_SEMICOLON, Key::SEMICOLON);
            CONV_CASE_INV(GLFW_KEY_EQUAL, Key::EQUAL);
            CONV_CASE_INV(GLFW_KEY_A, Key::A);
            CONV_CASE_INV(GLFW_KEY_B, Key::B);
            CONV_CASE_INV(GLFW_KEY_C, Key::C);
            CONV_CASE_INV(GLFW_KEY_D, Key::D);
            CONV_CASE_INV(GLFW_KEY_E, Key::E);
            CONV_CASE_INV(GLFW_KEY_F, Key::F);
            CONV_CASE_INV(GLFW_KEY_G, Key::G);
            CONV_CASE_INV(GLFW_KEY_H, Key::H);
            CONV_CASE_INV(GLFW_KEY_I, Key::I);
            CONV_CASE_INV(GLFW_KEY_J, Key::J);
            CONV_CASE_INV(GLFW_KEY_K, Key::K);
            CONV_CASE_INV(GLFW_KEY_L, Key::L);
            CONV_CASE_INV(GLFW_KEY_M, Key::M);
            CONV_CASE_INV(GLFW_KEY_N, Key::N);
            CONV_CASE_INV(GLFW_KEY_O, Key::O);
            CONV_CASE_INV(GLFW_KEY_P, Key::P);
            CONV_CASE_INV(GLFW_KEY_Q, Key::Q);
            CONV_CASE_INV(GLFW_KEY_R, Key::R);
            CONV_CASE_INV(GLFW_KEY_S, Key::S);
            CONV_CASE_INV(GLFW_KEY_T, Key::T);
            CONV_CASE_INV(GLFW_KEY_U, Key::U);
            CONV_CASE_INV(GLFW_KEY_V, Key::V);
            CONV_CASE_INV(GLFW_KEY_W, Key::W);
            CONV_CASE_INV(GLFW_KEY_X, Key::X);
            CONV_CASE_INV(GLFW_KEY_Y, Key::Y);
            CONV_CASE_INV(GLFW_KEY_Z, Key::Z);
            CONV_CASE_INV(GLFW_KEY_LEFT_BRACKET, Key::LEFT_BRACKET);
            CONV_CASE_INV(GLFW_KEY_BACKSLASH, Key::BACKSLASH);
            CONV_CASE_INV(GLFW_KEY_RIGHT_BRACKET, Key::RIGHT_BRACKET);
            CONV_CASE_INV(GLFW_KEY_GRAVE_ACCENT, Key::GRAVE_ACCENT);
            CONV_CASE_INV(GLFW_KEY_WORLD_1, Key::WORLD1);
            CONV_CASE_INV(GLFW_KEY_WORLD_2, Key::WORLD2);
            CONV_CASE_INV(GLFW_KEY_ESCAPE, Key::ESCAPE);
            CONV_CASE_INV(GLFW_KEY_ENTER, Key::ENTER);
            CONV_CASE_INV(GLFW_KEY_TAB, Key::TAB);
            CONV_CASE_INV(GLFW_KEY_BACKSPACE, Key::BACKSPACE);
            CONV_CASE_INV(GLFW_KEY_INSERT, Key::INSERT);
            CONV_CASE_INV(GLFW_KEY_DELETE, Key::DELETE);
            CONV_CASE_INV(GLFW_KEY_RIGHT, Key::RIGHT);
            CONV_CASE_INV(GLFW_KEY_LEFT, Key::LEFT);
            CONV_CASE_INV(GLFW_KEY_DOWN, Key::DOWN);
            CONV_CASE_INV(GLFW_KEY_UP, Key::UP);
            CONV_CASE_INV(GLFW_KEY_PAGE_UP, Key::PAGE_UP);
            CONV_CASE_INV(GLFW_KEY_PAGE_DOWN, Key::PAGE_DOWN);
            CONV_CASE_INV(GLFW_KEY_HOME, Key::HOME);
            CONV_CASE_INV(GLFW_KEY_END, Key::END);
            CONV_CASE_INV(GLFW_KEY_CAPS_LOCK, Key::CAPS_LOCK);
            CONV_CASE_INV(GLFW_KEY_SCROLL_LOCK, Key::SCROLL_LOCK);
            CONV_CASE_INV(GLFW_KEY_NUM_LOCK, Key::NUM_LOCK);
            CONV_CASE_INV(GLFW_KEY_PRINT_SCREEN, Key::PRINT_SCREEN);
            CONV_CASE_INV(GLFW_KEY_PAUSE, Key::PAUSE);
            CONV_CASE_INV(GLFW_KEY_F1, Key::F1);
            CONV_CASE_INV(GLFW_KEY_F2, Key::F2);
            CONV_CASE_INV(GLFW_KEY_F3, Key::F3);
            CONV_CASE_INV(GLFW_KEY_F4, Key::F4);
            CONV_CASE_INV(GLFW_KEY_F5, Key::F5);
            CONV_CASE_INV(GLFW_KEY_F6, Key::F6);
            CONV_CASE_INV(GLFW_KEY_F7, Key::F7);
            CONV_CASE_INV(GLFW_KEY_F8, Key::F8);
            CONV_CASE_INV(GLFW_KEY_F9, Key::F9);
            CONV_CASE_INV(GLFW_KEY_F10, Key::F10);
            CONV_CASE_INV(GLFW_KEY_F11, Key::F11);
            CONV_CASE_INV(GLFW_KEY_F12, Key::F12);
            CONV_CASE_INV(GLFW_KEY_F13, Key::F13);
            CONV_CASE_INV(GLFW_KEY_F14, Key::F14);
            CONV_CASE_INV(GLFW_KEY_F15, Key::F15);
            CONV_CASE_INV(GLFW_KEY_F16, Key::F16);
            CONV_CASE_INV(GLFW_KEY_F17, Key::F17);
            CONV_CASE_INV(GLFW_KEY_F18, Key::F18);
            CONV_CASE_INV(GLFW_KEY_F19, Key::F19);
            CONV_CASE_INV(GLFW_KEY_F20, Key::F20);
            CONV_CASE_INV(GLFW_KEY_F21, Key::F21);
            CONV_CASE_INV(GLFW_KEY_F22, Key::F22);
            CONV_CASE_INV(GLFW_KEY_F23, Key::F23);
            CONV_CASE_INV(GLFW_KEY_F24, Key::F24);
            CONV_CASE_INV(GLFW_KEY_F25, Key::F25);
            CONV_CASE_INV(GLFW_KEY_KP_0, Key::NUMPAD0);
            CONV_CASE_INV(GLFW_KEY_KP_1, Key::NUMPAD1);
            CONV_CASE_INV(GLFW_KEY_KP_2, Key::NUMPAD2);
            CONV_CASE_INV(GLFW_KEY_KP_3, Key::NUMPAD3);
            CONV_CASE_INV(GLFW_KEY_KP_4, Key::NUMPAD4);
            CONV_CASE_INV(GLFW_KEY_KP_5, Key::NUMPAD5);
            CONV_CASE_INV(GLFW_KEY_KP_6, Key::NUMPAD6);
            CONV_CASE_INV(GLFW_KEY_KP_7, Key::NUMPAD7);
            CONV_CASE_INV(GLFW_KEY_KP_8, Key::NUMPAD8);
            CONV_CASE_INV(GLFW_KEY_KP_9, Key::NUMPAD9);
            CONV_CASE_INV(GLFW_KEY_KP_DECIMAL, Key::NUMPAD_DECIMAL);
            CONV_CASE_INV(GLFW_KEY_KP_DIVIDE, Key::NUMPAD_DIVIDE);
            CONV_CASE_INV(GLFW_KEY_KP_MULTIPLY, Key::NUMPAD_MULTIPLY);
            CONV_CASE_INV(GLFW_KEY_KP_SUBTRACT, Key::NUMPAD_SUBSTRACT);
            CONV_CASE_INV(GLFW_KEY_KP_ADD, Key::NUMPAD_ADD);
            CONV_CASE_INV(GLFW_KEY_KP_ENTER, Key::NUMPAD_ENTER);
            CONV_CASE_INV(GLFW_KEY_KP_EQUAL, Key::NUMPAD_EQUAL);
            CONV_CASE_INV(GLFW_KEY_LEFT_SHIFT, Key::LEFT_SHIFT);
            CONV_CASE_INV(GLFW_KEY_LEFT_CONTROL, Key::LEFT_CONTROL);
            CONV_CASE_INV(GLFW_KEY_LEFT_ALT, Key::LEFT_ALT);
            CONV_CASE_INV(GLFW_KEY_LEFT_SUPER, Key::LEFT_SUPER);
            CONV_CASE_INV(GLFW_KEY_RIGHT_SHIFT, Key::RIGHT_SHIFT);
            CONV_CASE_INV(GLFW_KEY_RIGHT_CONTROL, Key::RIGHT_CONTROL);
            CONV_CASE_INV(GLFW_KEY_RIGHT_ALT, Key::RIGHT_ALT);
            CONV_CASE_INV(GLFW_KEY_RIGHT_SUPER, Key::RIGHT_SUPER);
            CONV_CASE_INV(GLFW_KEY_MENU, Key::MENU);
        default:
            throw std::runtime_error("Unrecognized!");
        }
    }

    MouseButton MouseButtonToOkami(int button) {
        switch (button) {
            CONV_CASE(GLFW_MOUSE_BUTTON_LEFT, MouseButton::LEFT);
            CONV_CASE(GLFW_MOUSE_BUTTON_RIGHT, MouseButton::RIGHT);
            CONV_CASE(GLFW_MOUSE_BUTTON_MIDDLE, MouseButton::MIDDLE);
            CONV_CASE(GLFW_MOUSE_BUTTON_4, MouseButton::BUTTON4);
            CONV_CASE(GLFW_MOUSE_BUTTON_5, MouseButton::BUTTON5);
            CONV_CASE(GLFW_MOUSE_BUTTON_6, MouseButton::BUTTON6);
            CONV_CASE(GLFW_MOUSE_BUTTON_7, MouseButton::BUTTON7);
            CONV_CASE(GLFW_MOUSE_BUTTON_8, MouseButton::BUTTON8);
        default:
            throw std::runtime_error("Unrecognized!");    
        }
    }

    int MouseButtonToGLFW(MouseButton button) {
        switch (button) {
            CONV_CASE_INV(GLFW_MOUSE_BUTTON_LEFT, MouseButton::LEFT);
            CONV_CASE_INV(GLFW_MOUSE_BUTTON_RIGHT, MouseButton::RIGHT);
            CONV_CASE_INV(GLFW_MOUSE_BUTTON_MIDDLE, MouseButton::MIDDLE);
            CONV_CASE_INV(GLFW_MOUSE_BUTTON_4, MouseButton::BUTTON4);
            CONV_CASE_INV(GLFW_MOUSE_BUTTON_5, MouseButton::BUTTON5);
            CONV_CASE_INV(GLFW_MOUSE_BUTTON_6, MouseButton::BUTTON6);
            CONV_CASE_INV(GLFW_MOUSE_BUTTON_7, MouseButton::BUTTON7);
            CONV_CASE_INV(GLFW_MOUSE_BUTTON_8, MouseButton::BUTTON8);
        default:
            throw std::runtime_error("Unrecognized!");    
        }
    }

    KeyAction KeyActionToOkami(int action) {
        switch (action) {
            CONV_CASE(GLFW_PRESS, KeyAction::PRESS);
            CONV_CASE(GLFW_RELEASE, KeyAction::RELEASE);
            CONV_CASE(GLFW_REPEAT, KeyAction::REPEAT);
        default:
            throw std::runtime_error("Unrecognized!");
        }
    }

    KeyState KeyStateToOkami(int state) {
        switch (state) {
            CONV_CASE(GLFW_PRESS, KeyState::PRESS);
            CONV_CASE(GLFW_RELEASE, KeyState::RELEASE);
        default:
            throw std::runtime_error("Unrecognized!");
        }
    }

    int KeyStateToGLFW(KeyState state) {
        switch (state) {
            CONV_CASE_INV(GLFW_PRESS, KeyState::PRESS);
            CONV_CASE_INV(GLFW_RELEASE, KeyState::RELEASE);
        default:
            throw std::runtime_error("Unrecognized!");
        }
    }

    int KeyActionToGLFW(KeyAction action) {
        switch (action) {
            CONV_CASE_INV(GLFW_PRESS, KeyAction::PRESS);
            CONV_CASE_INV(GLFW_RELEASE, KeyAction::RELEASE);
            CONV_CASE_INV(GLFW_REPEAT, KeyAction::REPEAT);
        default:
            throw std::runtime_error("Unrecognized!");
        }
    }

    KeyModifiers KeyModifiersToOkami(int keyModifiers) {
        return 
            KeyModifier::ALT * ((keyModifiers & GLFW_MOD_ALT) > 0) |
            KeyModifier::CAPS_LOCK * ((keyModifiers & GLFW_MOD_CAPS_LOCK) > 0) |
            KeyModifier::CONTROL * ((keyModifiers & GLFW_MOD_CONTROL) > 0) |
            KeyModifier::NUM_LOCK * ((keyModifiers & GLFW_MOD_NUM_LOCK) > 0) |
            KeyModifier::SHIFT * ((keyModifiers & GLFW_MOD_SHIFT) > 0) |
            KeyModifier::SUPER * ((keyModifiers & GLFW_MOD_SUPER) > 0);
    }

    int KeyModifiersToGLFW(KeyModifiers keyModifiers) {
        return 
            GLFW_MOD_ALT * ((keyModifiers & KeyModifier::ALT) > 0) |
            GLFW_MOD_CAPS_LOCK * ((keyModifiers & KeyModifier::CAPS_LOCK) > 0) |
            GLFW_MOD_CONTROL * ((keyModifiers & KeyModifier::CONTROL) > 0) |
            GLFW_MOD_NUM_LOCK * ((keyModifiers & KeyModifier::NUM_LOCK) > 0) |
            GLFW_MOD_SHIFT * ((keyModifiers & KeyModifier::SHIFT) > 0) |
            GLFW_MOD_SUPER * ((keyModifiers & KeyModifier::SUPER) > 0);
    }

    void GLFWKeyCallback(GLFWwindow* window,
        int key, int scancode, int action, int mods) {
        auto window_ = reinterpret_cast<WindowGLFW*>(
            glfwGetWindowUserPointer(window));
        window_->OnKeyEvent(
            KeyToOkami(key), 
            scancode, 
            KeyActionToOkami(action), 
            KeyModifiersToOkami(mods));
    }

    void GLFWCharCallback(GLFWwindow* window, unsigned int codepoint) {
        auto window_ = reinterpret_cast<WindowGLFW*>(
            glfwGetWindowUserPointer(window));
        window_->OnCharEvent(codepoint);
    }

    void GLFWScrollCallback(GLFWwindow* window, 
        double xoffset, double yoffset) {
        auto window_ = reinterpret_cast<WindowGLFW*>(
            glfwGetWindowUserPointer(window));
        window_->OnScrollEvent(xoffset, yoffset);
    }

    void GLFWMouseButtonCallback(GLFWwindow* window,
        int button, int action, int mods) {
        auto window_ = reinterpret_cast<WindowGLFW*>(
            glfwGetWindowUserPointer(window));
        window_->OnMouseButtonEvent(
            MouseButtonToOkami(button), 
            KeyActionToOkami(action), 
            KeyModifiersToOkami(mods));
    }

    void GLFWCursorEnterCallback(GLFWwindow* window, int entered) {
        auto window_ = reinterpret_cast<WindowGLFW*>(
            glfwGetWindowUserPointer(window));
        window_->OnCursorEnterEvent(entered);
    }

    void GLFWCharModCallback(GLFWwindow* window,
        unsigned int codepoint, int mods) {
        auto window_ = reinterpret_cast<WindowGLFW*>(
            glfwGetWindowUserPointer(window));
        window_->OnCharModEvent(codepoint, mods);
    }

    void GLFWDropCallback(GLFWwindow* window, 
        int path_count, const char* paths[]) {
        auto window_ = reinterpret_cast<WindowGLFW*>(
            glfwGetWindowUserPointer(window));
        window_->OnDropEvent(path_count, paths);
    }

    void GLFWCursorPosCallback(GLFWwindow* window,
        double xpos, double ypos) {
        auto window_ = reinterpret_cast<WindowGLFW*>(
            glfwGetWindowUserPointer(window));
        window_->OnCursorPosEvent(xpos, ypos);
    }

    WindowGLFW::WindowGLFW(
        const WindowParams& params, 
        const RealtimeGraphicsParams& graphicsParams) :
        mParams(params), 
        mGraphicsParams(graphicsParams),
        mWindowPollEvent(marl::Event::Mode::Manual) {

        GLFWwindow* window = glfwCreateWindow(
            params.mBackbufferWidth, 
            params.mBackbufferHeight,
            params.mWindowTitle.c_str(),
            nullptr, 
            nullptr);

        glfwSetWindowSizeLimits(window, 320, 240, GLFW_DONT_CARE, GLFW_DONT_CARE);

        if (!window) {
            throw std::runtime_error("Failed to create window!");
        }

        mWindow = window;
        mParams = params;

        glfwSetWindowUserPointer(mWindow, this);

        glfwSetKeyCallback(
            mWindow, &GLFWKeyCallback);
        glfwSetCharCallback(
            mWindow, &GLFWCharCallback);
        glfwSetScrollCallback(
            mWindow, &GLFWScrollCallback);
        glfwSetMouseButtonCallback(
            mWindow, &GLFWMouseButtonCallback);
        glfwSetCursorEnterCallback(
            mWindow, &GLFWCursorEnterCallback);
        glfwSetCharModsCallback(
            mWindow, &GLFWCharModCallback);
        glfwSetDropCallback(
            mWindow, &GLFWDropCallback);
        glfwSetCursorPosCallback(
            mWindow, &GLFWCursorPosCallback);

        float scaleX, scaleY;
        glfwGetWindowContentScale(mWindow, &scaleX, &scaleY);

        mContentScale = (scaleX + scaleY) / 2.0;

        mNativeWindowStruct = MakeNativeWindowStruct();
    }

    WindowGLFW::~WindowGLFW() {
        Close();
    }

    void DisplayGLFW::Startup(const RealtimeGraphicsParams& params) {
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize glfw!");
        }

        glfwSetErrorCallback(&HandleGLFWError);

        int glfwApiHint = GLFW_NO_API;

        glfwWindowHint(GLFW_CLIENT_API, glfwApiHint);
        if (glfwApiHint == GLFW_OPENGL_API)
        {
            glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);

#if PLATFORM_MACOS
            // MacOS only supports OpenGL 4.1
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
#else
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
#endif
        }
    }

    DisplayGLFW::DisplayGLFW(
        core::ResourceInterface* resources,
        const RealtimeGraphicsParams& params) :  
        mParams(params),
        mResources(resources) {
    }

    ICursor* DisplayGLFW::CreateStandardCursor(StandardCursor cursor) {
        GLFWcursor* glfwResult = nullptr;
        
        switch (cursor) {
            case StandardCursor::ARROW:
                glfwResult = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
                break;
            case StandardCursor::CROSSHAIR:
                glfwResult = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);
                break;
            case StandardCursor::HAND:
                glfwResult = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
                break;
            case StandardCursor::HRESIZE:
                glfwResult = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
                break;
            case StandardCursor::IBEAM:
                glfwResult = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
                break;
            case StandardCursor::VRESIZE:
                glfwResult = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
                break;
#if GLFW_HAS_NEW_CURSORS
            case StandardCursor::RESIZE_ALL:
                glfwResults = glfwCreateStandardCursor(GLFW_RESIZE_ALL_CURSOR);
                break;
            case StandardCursor::RESIZE_NESW:
                glfwResults = glfwCreateStandardCursor(GLFW_RESIZE_NESW_CURSOR);
                break;
            case StandardCursor::RESIZE_NWSE:
                glfwResult = glfwCreateStandardCursor(GLFW_RESIZE_NWSE_CURSOR);
                break;
            case StandardCursor::NOT_ALLOWED:
                glfwResult = glfwCreateStandardCursor(GLFW_NOT_ALLOWED_CURSOR);
                break;
#else
            case StandardCursor::RESIZE_ALL:
            case StandardCursor::RESIZE_NESW:
            case StandardCursor::RESIZE_NWSE:
            case StandardCursor::NOT_ALLOWED:    
                glfwResult = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
                break;
#endif
            default:
                throw std::runtime_error("Unrecognized cursor!");
        }

        auto result = std::make_unique<CursorGLFW>(glfwResult, ++mCurrentCursorsId);
        
        auto it = mCursors.emplace(result->GetId(), std::move(result));
        return it.first->second.get();
    }

    void DisplayGLFW::DestroyCursor(ICursor* cursor) {
        auto it = mCursors.find(cursor->GetId());

        if (it != mCursors.end()) {
            mCursors.erase(it);
        }
    }

    void DisplayGLFW::DestroyWindow(IWindow* window) {
        auto it = mWindows.find(window->GetId());

        if (it != mWindows.end()) {
            mWindows.erase(it);
        }
    }

    IWindow* DisplayGLFW::CreateWindow(
        const WindowParams& params) {

        auto ptr = std::make_unique<WindowGLFW>(params, mParams);
        auto result = ptr.get();
        auto size = ptr->GetFramebufferSize();

        RenderPass pass;
        pass.mAttributes[0] = RenderAttribute::COLOR;
        pass.mAttributeCount = 1;

        RenderCanvasDesc desc;
        desc.mWidth = size.x;
        desc.mHeight = size.y;
        desc.mWindow = ptr.get();
        desc.mPassInfo = pass;

        RenderCanvas canvas(desc);

        ptr->mRenderCanvas = mResources->Add<RenderCanvas>(
            std::move(canvas));
        ptr->mId = ++mCurrentWindowId;

        mWindows.emplace(ptr->mId, std::move(ptr));

        return result;
    }

    void DisplayGLFW::Startup(marl::WaitGroup& waitGroup) {
        Startup(mParams);
    }

    void DisplayGLFW::RegisterInterfaces(core::InterfaceCollection& interfaces) {
        interfaces.Add<IDisplay>(this);
    }

    bool WindowGLFW::ShouldClose() const {
        return glfwWindowShouldClose(mWindow);
    }

    void WindowGLFW::Close() {
        if (mWindow)
            glfwDestroyWindow(mWindow);

        mWindow = nullptr;
        mRenderCanvas = nullptr;
    }

    void DisplayGLFW::Shutdown() {
        for (auto& [id, window] : mWindows) {
            window->Close();
        }
        mCursors.clear();
        mWindows.clear();

        glfwTerminate();
    }

    void DisplayGLFW::SetFrame(core::Frame& frame) {
    }

    void DisplayGLFW::LoadResources(marl::WaitGroup& waitGroup) {
    }

    void DisplayGLFW::RequestSync(core::SyncObject& syncObject) {
        for (auto& [id, window] : mWindows) {
            window->mWindowPollEvent.clear();
        }
    }

    void DisplayGLFW::Fork(core::Frame& frame, 
        core::SyncObject& syncObject,
        const core::Time& time) {

        for (auto it = mWindows.begin(); it != mWindows.end();) {
            auto current = it++;

            if (current->second->IsClosed()) {
                mWindows.erase(current);
            }
        }
        
        for (auto& [id, window] : mWindows) {
            if (window->bResizeRequested) {
                glfwSetWindowSize(
                    window->mWindow, 
                    window->mResize.x, 
                    window->mResize.y);
                window->bResizeRequested = false;
            }

            window->UpdateCaptures();
        }

        glfwPollEvents();

        for (auto& [id, window] : mWindows) {
            window->UpdateCaptures();
            window->mWindowPollEvent.signal();
        }
    }

    void DisplayGLFW::Join(core::Frame& frame) {
    }

    void DisplayGLFW::Wait() {
    }

    void WindowGLFW::SetFramebufferSize(uint width, uint height) {
        bResizeRequested = true;
        mResize.x = width;
        mResize.y = height;
        mRenderCanvas->Resize(width, height);
    }

    const WindowParams& WindowGLFW::GetDesc() const {
        return mParams;
    }

    const void* WindowGLFW::GetNativeWindow() const {
        return &mNativeWindowStruct;
    }

    void* WindowGLFW::GetBackend() const {
        return mWindow;
    }

    const char* WindowGLFW::GetClipboardText() {
        return glfwGetClipboardString(mWindow);
    }

    void WindowGLFW::SetClipboardText(const char* text) {
        glfwSetClipboardString(mWindow, text);
    }

    NativeWindow WindowGLFW::MakeNativeWindowStruct() const {
    #if PLATFORM_WIN32
		DG::Win32NativeWindow Window{glfwGetWin32Window(mWindow)};
	#endif
	#if PLATFORM_LINUX
		DG::LinuxNativeWindow Window;
		Window.WindowId = glfwGetX11Window(mWindow);
		Window.pDisplay = glfwGetX11Display();
	#endif
	#if PLATFORM_MACOS
		DG::MacOSNativeWindow Window;
		Window.pNSView = GetNSWindowView(mWindow);
	#endif

        return Window;
    }

    glm::i32vec2 WindowGLFW::GetFramebufferSize() const {
        glm::i32vec2 size;
        glfwGetFramebufferSize(mWindow, &size.x, &size.y);
        return size;
    }

    glm::i32vec2 WindowGLFW::GetWindowSize() const {
        glm::i32vec2 size;
        glfwGetWindowSize(mWindow, &size.x, &size.y);
        return size;
    }

    bool WindowGLFW::GetIsFullscreen() const {
        return mParams.bFullscreen;
    }

    GraphicsBackend DisplayGLFW::GetRequestedBackend() const {
        return mParams.mBackend;
    }

    void WindowGLFW::AddCapture(
        IInputCapture* capture, CallbackPriority priority) {
        if (!capture)
            return;

        for (auto& it : mCaptureObjects) {
            if (it.mInterface == capture) {
                it.mRefCount++;
                it.mPriority = std::max(it.mPriority, priority);
                return;
            }
        }

        mCaptureObjects.emplace_back(
            CaptureObject{
                priority,
                1,
                capture
        });
        
        bCaptureObjectsDirty = true;
    }

    void WindowGLFW::RemoveCapture(IInputCapture* capture) {
        if (!capture)
            return;

        for (auto it = mCaptureObjects.begin(); it != mCaptureObjects.end(); ++it) {
            if (it->mInterface == capture) {
                it->mRefCount--;
                if (it->mRefCount == 0) {
                    mCaptureObjects.erase(it);
                }
                return;
            }
        }
    }

    void WindowGLFW::UpdateCaptures() {
        if (bCaptureObjectsDirty) {
            std::sort(mCaptureObjects.begin(),
                mCaptureObjects.end(),
                [](const CaptureObject& o1, const CaptureObject& o2) {
                    return o1.mPriority < o2.mPriority;
            });

            bCaptureObjectsDirty = false;
        }

        if (mMouseFocus) {
            if (!mMouseFocus->ShouldCaptureMouse()) {
                mMouseFocus = nullptr;
            }
        }

        if (mKeyboardFocus) {
            if (!mKeyboardFocus->ShouldCaptureKeyboard()) {
                mKeyboardFocus = nullptr;
            }
        }

        for (auto& it : mCaptureObjects) {
            if (!mMouseFocus) {
                if (it.mInterface->ShouldCaptureMouse()) {
                    mMouseFocus = it.mInterface;
                }
            }
            if (!mKeyboardFocus) {
                if (it.mInterface->ShouldCaptureKeyboard()) {
                    mKeyboardFocus = it.mInterface;
                }
            }
        }
    }

    core::delegate_handle_t 
        WindowGLFW::AddMouseButtonCallback(
            const mouse_button_callback_t& c, 
            CallbackPriority p,
            IInputCapture* capture) {
        AddCapture(capture, p);
        return mMouseButtonEvent.Add(c, capture, (double)p);
    }
    core::delegate_handle_t 
        WindowGLFW::AddKeyCallback(
            const key_callback_t& c, 
            CallbackPriority p,
            IInputCapture* capture) {
        AddCapture(capture, p);
        return mKeyEvent.Add(c, capture, (double)p);
    }
    core::delegate_handle_t 
        WindowGLFW::AddCharCallback(
            const char_callback_t& c, 
            CallbackPriority p,
            IInputCapture* capture) {
        AddCapture(capture, p);
        return mCharEvent.Add(c, capture, (double)p);
    }
    core::delegate_handle_t 
        WindowGLFW::AddScrollCallback(
            const scroll_callback_t& c, 
            CallbackPriority p,
            IInputCapture* capture) {
        AddCapture(capture, p);
        return mScrollEvent.Add(c, capture, (double)p);
    }
    core::delegate_handle_t
        WindowGLFW::AddCharModCallback(
            const char_mod_callback_t& c, 
            CallbackPriority p,
            IInputCapture* capture) {
        AddCapture(capture, p);
        return mCharModEvent.Add(c, capture, (double)p);
    }
    core::delegate_handle_t
        WindowGLFW::AddCursorPosCallback(
            const cursor_pos_callback_t& c, 
            CallbackPriority p,
            IInputCapture* capture) {
        AddCapture(capture, p);
        return mCursorPosEvent.Add(c, capture, (double)p);
    }
    core::delegate_handle_t
        WindowGLFW::AddDropCallback(
            const drop_callback_t& c, 
            CallbackPriority p) {
        return mDropEvent.Add(c, (double)p);
    }
    core::delegate_handle_t
        WindowGLFW::AddCursorEnterCallback(
            const cursor_enter_callback_t& c, 
            CallbackPriority p,
            IInputCapture* capture) {
        AddCapture(capture, p);
        return mCursorEnterEvent.Add(c, capture, (double)p);
    }
        
    void WindowGLFW::RemoveMouseButtonCallback(core::delegate_handle_t h) {
        RemoveCapture(mMouseButtonEvent.Remove(h));
    }
    void WindowGLFW::RemoveKeyCallback(core::delegate_handle_t h) {
        RemoveCapture(mKeyEvent.Remove(h));
    }
    void WindowGLFW::RemoveCharCallback(core::delegate_handle_t h) {
        RemoveCapture(mCharEvent.Remove(h));
    }
    void WindowGLFW::RemoveScrollCallback(core::delegate_handle_t h) {
        RemoveCapture(mScrollEvent.Remove(h));
    }
    void WindowGLFW::RemoveCharModCallback(core::delegate_handle_t h) {
        RemoveCapture(mCharModEvent.Remove(h));
    }
    void WindowGLFW::RemoveCursorPosCallback(core::delegate_handle_t h) {
        RemoveCapture(mCursorPosEvent.Remove(h));
    }
    void WindowGLFW::RemoveDropCallback(core::delegate_handle_t h) {
        mDropEvent.Remove(h);
    }
    void WindowGLFW::RemoveCursorEnterCallback(core::delegate_handle_t h) {
        RemoveCapture(mCursorEnterEvent.Remove(h));
    }

    void WindowGLFW::WaitForInput() {
        mWindowPollEvent.wait();
    }

    void* WindowGLFW::GetWin32Window() {
#if defined(_WIN32)
        return glfwGetWin32Window(mWindow);
#else
        return nullptr;
#endif
    }

    glm::dvec2 WindowGLFW::GetCursorPos() const {
        glm::dvec2 result;
        glfwGetCursorPos(mWindow, &result.x, &result.y);
        return result;
    }

    Handle<RenderCanvas> WindowGLFW::GetCanvas() const {
        return mRenderCanvas;
    }

    void WindowGLFW::SetCursor(ICursor* cursor) {
        GLFWcursor* _cursor = reinterpret_cast<GLFWcursor*>(cursor->GetBackend());
        glfwSetCursor(mWindow, _cursor);
    }

    void WindowGLFW::RegisterInterfaces(
        core::InterfaceCollection& interfaces) {    
    }

    uint WindowGLFW::GetId() const {
        return mId;
    }

    void WindowGLFW::SetCursorMode(CursorMode cursor) {
        switch (cursor) {
            case CursorMode::NORMAL:
                glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                break;
            case CursorMode::HIDDEN:
                glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
                break;
            case CursorMode::DISABLED:
                glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                break;
        }
    }

    core::KeyState WindowGLFW::GetState(core::MouseButton mouseButton) const {
        return KeyStateToOkami(
            glfwGetMouseButton(mWindow, MouseButtonToGLFW(mouseButton)));
    }

    core::KeyState WindowGLFW::GetState(core::Key key) const {
        return KeyStateToOkami(
            glfwGetKey(mWindow, KeyToGLFW(key)));
    }

    void WindowGLFW::SetRawMouseMotion(bool enabled) {
        if (enabled)
            glfwSetInputMode(mWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        else 
            glfwSetInputMode(mWindow, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
    }

    bool WindowGLFW::IsRawMouseMotionSupported() const {
        return glfwRawMouseMotionSupported();
    }

    IInputCapture* WindowGLFW::GetMouseFocus() {
        return mMouseFocus;
    }

    IInputCapture* WindowGLFW::GetKeyboardFocus() {
        return mKeyboardFocus;
    }

    float WindowGLFW::GetContentScale() const {
        return mContentScale;
    }

    template <typename... Args>
    void WindowGLFW::InvokeEvent(IInputCapture*& captureGroup,
        core::OrderedEvent<IInputCapture, Args...>& event,
        Args... __args) {
        if (captureGroup) {
            event.InvokeOnly(captureGroup, __args...);
        } else {
            auto capture = event(__args...);
            if (capture) {
                captureGroup = capture;
            }
        }
    }

    void WindowGLFW::OnKeyEvent(
        Key key, int scancode, KeyAction action, KeyModifiers mods) {
        InvokeEvent(mKeyboardFocus, mKeyEvent, 
            static_cast<IWindow*>(this), key, scancode, action, mods);
    }
    void WindowGLFW::OnCharEvent(
        unsigned int codepoint) {
        InvokeEvent(mKeyboardFocus, mCharEvent, 
            static_cast<IWindow*>(this), codepoint);
    }
    void WindowGLFW::OnScrollEvent(
        double xoffset, double yoffset) {
        InvokeEvent(mMouseFocus, mScrollEvent, 
            static_cast<IWindow*>(this), xoffset, yoffset);
    }
    void WindowGLFW::OnMouseButtonEvent(
        MouseButton button, KeyAction action, KeyModifiers mods) {
        InvokeEvent(mMouseFocus, mMouseButtonEvent, 
            static_cast<IWindow*>(this), button, action, mods);
    }
    void WindowGLFW::OnCharModEvent(
        unsigned int codepoint, KeyModifiers mods) {
        InvokeEvent(mKeyboardFocus, mCharModEvent,
            static_cast<IWindow*>(this), codepoint, mods);
    }
    void WindowGLFW::OnDropEvent(
        int path_count, const char* paths[]) {
        mDropEvent(this, path_count, paths);
    }
    void WindowGLFW::OnCursorPosEvent(
        double xpos, double ypos) {
        InvokeEvent(mMouseFocus, mCursorPosEvent,
            static_cast<IWindow*>(this), xpos, ypos);
    }
    void WindowGLFW::OnCursorEnterEvent(
        int entered) {
        InvokeEvent(mMouseFocus, mCursorEnterEvent,
            static_cast<IWindow*>(this), entered);
    }

    CursorGLFW::CursorGLFW(GLFWcursor* cursor, uint id) : 
        mId(id),
        mCursor(cursor) {
    }

    CursorGLFW::~CursorGLFW() {
        glfwDestroyCursor(mCursor);
    }

    uint CursorGLFW::GetId() const {
        return mId;
    }

    void* CursorGLFW::GetBackend() {
        return mCursor;
    }
}

#endif