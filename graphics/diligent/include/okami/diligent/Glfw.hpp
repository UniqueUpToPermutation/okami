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
}

#if USE_GLFW
#include <GLFW/glfw3.h>

namespace okami::graphics::diligent {

    class CursorGLFW final :
        public ICursor {
    public:
        uint mId = 0;
        GLFWcursor* mCursor = nullptr;
       
        uint GetId() const override;
        void* GetBackend() override;

        CursorGLFW(GLFWcursor* cursor, uint id);
        ~CursorGLFW();
    };

    class WindowGLFW final :
        public IWindow {
    public:
        struct CaptureObject {
            core::CallbackPriority mPriority;
            uint mRefCount = 0;
            core::IInputCapture* mInterface = nullptr;
        };

        std::vector<CaptureObject> mCaptureObjects;
        bool bCaptureObjectsDirty = false;
        core::IInputCapture* mMouseFocus = nullptr;
        core::IInputCapture* mKeyboardFocus = nullptr;

        float mContentScale = 1.0;
        uint mId = 0;

        GLFWwindow* mWindow = nullptr;
		WindowParams mParams;
        RealtimeGraphicsParams mGraphicsParams;
        bool bResizeRequested = false;
        glm::uvec2 mResize;
        NativeWindow mNativeWindowStruct;
        Handle<RenderCanvas> mRenderCanvas;

        inline bool IsClosed() const {
            return mWindow == nullptr;
        }

        void AddCapture(core::IInputCapture* capture, core::CallbackPriority priority);
        void RemoveCapture(core::IInputCapture* capture);
        void UpdateCaptures();

        core::OrderedEvent<core::IInputCapture,
            IWindow*, core::Key, int, core::KeyAction, core::KeyModifiers> mKeyEvent;
        core::OrderedEvent<core::IInputCapture,
            IWindow*, unsigned int> mCharEvent;
        core::OrderedEvent<core::IInputCapture,
            IWindow*, double, double> mScrollEvent;
        core::OrderedEvent<core::IInputCapture,
            IWindow*, core::MouseButton, core::KeyAction, core::KeyModifiers> mMouseButtonEvent;
        core::OrderedEvent<core::IInputCapture,
            IWindow*, unsigned int, core::KeyModifiers> mCharModEvent;
        core::OrderedEvent<core::IInputCapture,
            IWindow*, double, double> mCursorPosEvent;
        core::OrderedEvent<core::IInputCapture,
            IWindow*, int> mCursorEnterEvent;
        core::OrderedEvent<void, 
            IWindow*, int, const char**> mDropEvent;

        marl::Event mWindowPollEvent;

        template <typename... Args>
        void InvokeEvent(core::IInputCapture*& captureGroup,
            core::OrderedEvent<core::IInputCapture, Args...>& event,
            Args... __args);

        void OnKeyEvent(
            core::Key key, int scancode, core::KeyAction action, core::KeyModifiers mods);
        void OnCharEvent(
            unsigned int codepoint);
        void OnScrollEvent(
            double xoffset, double yoffset);
        void OnMouseButtonEvent(
            core::MouseButton button, core::KeyAction action, core::KeyModifiers mods);
        void OnCharModEvent(
            unsigned int codepoint, core::KeyModifiers mods);
        void OnDropEvent(
            int path_count, const char* paths[]);
        void OnCursorPosEvent( 
            double xpos, double ypos);
        void OnCursorEnterEvent(
            int entered);

        WindowGLFW(
            const WindowParams& params,
            const RealtimeGraphicsParams& graphicsParams);
        ~WindowGLFW();

        bool ShouldClose() const override;
        void Close() override;

        void SetFramebufferSize(uint width, uint height) override;
        
        void* GetBackend() const override;
        NativeWindow MakeNativeWindowStruct() const;
        glm::i32vec2 GetFramebufferSize() const override;
        glm::i32vec2 GetWindowSize() const override;
        bool IsFullscreen() const override;
        bool IsPrimary() const override;

        core::delegate_handle_t 
            AddMouseButtonCallback(
                const core::mouse_button_callback_t&, 
                core::CallbackPriority,
                core::IInputCapture*) override;
        core::delegate_handle_t 
            AddKeyCallback(
                const core::key_callback_t&, 
                core::CallbackPriority,
                core::IInputCapture*) override;
        core::delegate_handle_t 
            AddCharCallback(
                const core::char_callback_t&, 
                core::CallbackPriority,
                core::IInputCapture*) override;
        core::delegate_handle_t 
            AddScrollCallback(
                const core::scroll_callback_t&, 
                core::CallbackPriority,
                core::IInputCapture*) override;
        core::delegate_handle_t
            AddCharModCallback(
                const core::char_mod_callback_t&, 
                core::CallbackPriority,
                core::IInputCapture*) override;
        core::delegate_handle_t
            AddCursorPosCallback(
                const core::cursor_pos_callback_t&, 
                core::CallbackPriority,
                core::IInputCapture*) override;
        core::delegate_handle_t
            AddDropCallback(
                const core::drop_callback_t&, 
                core::CallbackPriority) override;
        core::delegate_handle_t
            AddCursorEnterCallback(
                const core::cursor_enter_callback_t&, 
                core::CallbackPriority,
                core::IInputCapture*) override;
    
        void RemoveMouseButtonCallback(core::delegate_handle_t) override;
        void RemoveKeyCallback(core::delegate_handle_t) override;
        void RemoveCharCallback(core::delegate_handle_t) override;
        void RemoveScrollCallback(core::delegate_handle_t) override;
        void RemoveCharModCallback(core::delegate_handle_t) override;
        void RemoveCursorPosCallback(core::delegate_handle_t) override;
        void RemoveDropCallback(core::delegate_handle_t) override;
        void RemoveCursorEnterCallback(core::delegate_handle_t) override;

        void OnResize(int width, int height);
    
        core::IInputCapture* GetMouseFocus() override;
        core::IInputCapture* GetKeyboardFocus() override;

        float GetContentScale() const override;
        void WaitForInput() override;
        void* GetWin32Window() override;

        core::KeyState GetState(core::MouseButton mouseButton) const override;
        core::KeyState GetState(core::Key key) const override;
        void SetCursorMode(core::CursorMode cursor) override;
        core::CursorMode GetCursorMode() const override;
        void SetRawMouseMotion(bool enabled) override;
        bool IsRawMouseMotionSupported() const override;
        glm::dvec2 GetCursorPos() const override;
        void SetCursorPos(const glm::dvec2& pos) override;
        Handle<RenderCanvas> GetCanvas() const override;
        void SetCursor(ICursor* cursor) override;

        void RegisterInterfaces(core::InterfaceCollection& interfaces) override;
        const WindowParams& GetDesc() const override;
        const void* GetNativeWindow() const override;
        uint GetId() const override;

        const char* GetClipboardText() override;
        void SetClipboardText(const char* text) override;
        bool IsFocused() const override;
    };

    class DisplayGLFW final : 
        public core::ISystem, 
        public IDisplay {
    private:
        std::unordered_map<
            uint, std::unique_ptr<WindowGLFW>> mWindows;
        std::unordered_map<
            uint, std::unique_ptr<CursorGLFW>> mCursors;
        RealtimeGraphicsParams mParams;
        core::ResourceInterface* mResources;

        uint mCurrentWindowId = 0;
        uint mCurrentCursorsId = 0;

        void Startup(const RealtimeGraphicsParams& params);

    public:
        DisplayGLFW(
            core::ResourceInterface* resourceInterface,
            const RealtimeGraphicsParams& params);
        ~DisplayGLFW() = default;

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

        IWindow* CreateWindow(
            const WindowParams& params = WindowParams()) override;
        void DestroyWindow(IWindow* window) override;

        ICursor* CreateStandardCursor(StandardCursor cursor) override;
        void DestroyCursor(ICursor* cursor) override;

        GraphicsBackend GetRequestedBackend() const override;
    };
}

#endif