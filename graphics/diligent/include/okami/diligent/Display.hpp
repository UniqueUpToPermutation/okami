#pragma once

#include <okami/Graphics.hpp>

#if PLATFORM_WIN32
#   include <Win32NativeWindow.h>
#endif
#if PLATFORM_LINUX
#   define GLFW_EXPOSE_NATIVE_X11 true
#   include <LinuxNativeWindow.h>
#endif
#if PLATFORM_MACOS
#   include <MacOSNativeWindow.h>
#endif

namespace okami::graphics::diligent {
    namespace DG = Diligent;

#if PLATFORM_WIN32
    typedef DG::Win32NativeWindow NativeWindow;
#endif
#if PLATFORM_LINUX
    typedef DG::LinuxNativeWindow NativeWindow;
#endif
#if PLATFORM_MACOS
    typedef DG::MacOSNativeWindow NativeWindow;
#endif

    class INativeWindowProvider {
    public:
        virtual NativeWindow GetWindow() = 0;
        virtual void GLMakeContextCurrent() = 0;
        virtual void GLSwapBuffers(int swapInterval = 1) = 0;
    };
}

#if USE_GLFW
#include <GLFW/glfw3.h>

namespace okami::graphics::diligent {

    typedef std::function<bool(
        GLFWwindow* window,
        int key,
        int scancode,
        int action,
        int mods)> key_callback_t;

    typedef std::function<bool(
        GLFWwindow* window,
        unsigned int codepoint)> char_callback_t;

    typedef std::function<bool(
        GLFWwindow* window,
        double xoffset,
        double yoffset)> scroll_callback_t;

    typedef std::function<bool(
        GLFWwindow* window,
        int button,
        int action,
        int mods)> mouse_button_callback_t;

    typedef std::function<bool(
        GLFWwindow* window,
        int entered)> cursor_enter_callback_t;

    typedef std::function<bool(
        GLFWwindow* window,
        unsigned int codepoint,
        int mods)> char_mod_callback_t;

    typedef std::function<bool(
        GLFWwindow* window,
        int path_count,
        const char* paths[])> drop_callback_t;

    typedef std::function<bool(
        GLFWwindow* window,
        double xpos,
        double ypos)> cursor_pos_callback_t;

    enum class CallbackPriority {
        LOW = 0,
        MEDIUM = 1,
        HIGH = 2,
        GUI = 3
    };

    class IGLFWWindowProvider {
    public:
        virtual GLFWwindow* GetWindowGLFW() const = 0;

        virtual core::delegate_handle_t 
            AddMouseButtonCallback(
                const mouse_button_callback_t& callback, 
                CallbackPriority priority = CallbackPriority::MEDIUM) = 0;
        virtual core::delegate_handle_t 
            AddKeyCallback(
                const key_callback_t& calback,
                CallbackPriority priority = CallbackPriority::MEDIUM) = 0;
        virtual core::delegate_handle_t 
            AddCharCallback(
                const char_callback_t& callback, 
                CallbackPriority priority = CallbackPriority::MEDIUM) = 0;
        virtual core::delegate_handle_t 
            AddScrollCallback(
                const scroll_callback_t& callback,
                CallbackPriority priority = CallbackPriority::MEDIUM) = 0;
        virtual core::delegate_handle_t
            AddCharModCallback(
                const char_mod_callback_t& callback,
                CallbackPriority priority = CallbackPriority::MEDIUM) = 0;
        virtual core::delegate_handle_t
            AddCursorPosCallback(
                const cursor_pos_callback_t& callback,
                CallbackPriority priority = CallbackPriority::MEDIUM) = 0;
        virtual core::delegate_handle_t
            AddDropCallback(
                const drop_callback_t& callback,
                CallbackPriority priority = CallbackPriority::MEDIUM) = 0;
        virtual core::delegate_handle_t
            AddCursorEnterCallback(
                const cursor_enter_callback_t& callback,
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
        
        virtual void WaitForInput() = 0;
    };

    class DisplayGLFW final : 
        public core::ISystem, 
        public IDisplay,
        public INativeWindowProvider,
        public IGLFWWindowProvider {
    private:
        GLFWwindow* mWindow = nullptr;
		RealtimeGraphicsParams mParams;
        int mGLSwapInterval = -1;
        bool bResizeRequested = false;
        glm::uvec2 mResize;

        void Startup(const RealtimeGraphicsParams& params);

        core::OrderedEvent<
            GLFWwindow*, int, int, int, int> mKeyEvent;
        core::OrderedEvent<
            GLFWwindow*, unsigned int> mCharEvent;
        core::OrderedEvent<
            GLFWwindow*, double, double> mScrollEvent;
        core::OrderedEvent<
            GLFWwindow*, int, int, int> mMouseButtonEvent;
        core::OrderedEvent<
            GLFWwindow*, int> mMouseEnterEvent;
        core::OrderedEvent<
            GLFWwindow*, unsigned int, int> mCharModEvent;
        core::OrderedEvent<
            GLFWwindow*, int, const char**> mDropEvent;
        core::OrderedEvent<
            GLFWwindow*, double, double> mCursorPosEvent;
        core::OrderedEvent<
            GLFWwindow*, int> mCursorEnterEvent;

        GLFWmousebuttonfun mPrevMouseButton;
        GLFWcursorposfun mPrevCursorPos;
        GLFWcursorenterfun mPrevCursorEnter;
        GLFWscrollfun mPrevScroll;
        GLFWkeyfun mPrevKey;
        GLFWcharfun mPrevChar;
        GLFWcharmodsfun mPrevCharMods;
        GLFWdropfun mPrevDrop;

        marl::Event mWindowPollEvent;

    public:
        void OnKeyEvent(GLFWwindow* window, 
            int key, int scancode, int action, int mods);
        void OnCharEvent(GLFWwindow* window, 
            unsigned int codepoint);
        void OnScrollEvent(GLFWwindow* window, 
            double xoffset, double yoffset);
        void OnMouseButtonEvent(GLFWwindow* window, 
            int button, int action, int mods);
        void OnCharModEvent(GLFWwindow* window,
            unsigned int codepoint, int mods);
        void OnDropEvent(GLFWwindow* window,
            int path_count, const char* paths[]);
        void OnCursorPosEvent(GLFWwindow* window, 
            double xpos, double ypos);
        void OnCursorEnterEvent(GLFWwindow* window, 
            int entered);

        DisplayGLFW(const RealtimeGraphicsParams& params);

        bool ShouldClose() const override;
        void RegisterInterfaces(core::InterfaceCollection& interfaces) override;
        void Startup(marl::WaitGroup& waitGroup) override;
        void Shutdown() override;
        void LoadResources(marl::WaitGroup& waitGroup) override;
        void SetFrame(core::Frame& frame) override;
        void RequestSync(core::SyncObject& syncObject) override;
        void Fork(core::Frame& frame, 
            core::SyncObject& syncObject,
            const core::Time& time) override;
        void Join(core::Frame& frame) override;
        void Wait() override;
        void SetFramebufferSize(uint width, uint height) override;
        
        GLFWwindow* GetWindowGLFW() const override;
        NativeWindow GetWindow() override;
        void GLMakeContextCurrent() override;
        void GLSwapBuffers(int swapInterval) override;
        glm::i32vec2 GetFramebufferSize() const override;
        bool GetIsFullscreen() const override;
        GraphicsBackend GetRequestedBackend() const override;

        core::delegate_handle_t 
            AddMouseButtonCallback(
                const mouse_button_callback_t&, CallbackPriority) override;
        core::delegate_handle_t 
            AddKeyCallback(
                const key_callback_t&, CallbackPriority) override;
        core::delegate_handle_t 
            AddCharCallback(
                const char_callback_t&, CallbackPriority) override;
        core::delegate_handle_t 
            AddScrollCallback(
                const scroll_callback_t&, CallbackPriority) override;
        core::delegate_handle_t
            AddCharModCallback(
                const char_mod_callback_t&, CallbackPriority) override;
        core::delegate_handle_t
            AddCursorPosCallback(
                const cursor_pos_callback_t&, CallbackPriority) override;
        core::delegate_handle_t
            AddDropCallback(
                const drop_callback_t&, CallbackPriority) override;
        core::delegate_handle_t
            AddCursorEnterCallback(
                const cursor_enter_callback_t&, CallbackPriority) override;
    
        void RemoveMouseButtonCallback(core::delegate_handle_t) override;
        void RemoveKeyCallback(core::delegate_handle_t) override;
        void RemoveCharCallback(core::delegate_handle_t) override;
        void RemoveScrollCallback(core::delegate_handle_t) override;
        void RemoveCharModCallback(core::delegate_handle_t) override;
        void RemoveCursorPosCallback(core::delegate_handle_t) override;
        void RemoveDropCallback(core::delegate_handle_t) override;
        void RemoveCursorEnterCallback(core::delegate_handle_t) override;
    
        void WaitForInput() override;
    };
}

#endif