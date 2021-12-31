#include <okami/diligent/Display.hpp>

#if USE_GLFW

#if PLATFORM_MACOS
extern void* GetNSWindowView(GLFWwindow* wnd);
#endif

#include <GLFW/glfw3native.h>

#include <iostream>

void HandleGLFWError(int code, const char* msg) {
    std::cerr << "ERROR (GLFW code " << code << "): " << msg << std::endl;
}

namespace okami::graphics::diligent {

    void GLFWKeyCallback(GLFWwindow* window,
        int key, int scancode, int action, int mods) {
        auto display = reinterpret_cast<DisplayGLFW*>(
            glfwGetWindowUserPointer(window));
        display->OnKeyEvent(window, key, scancode, action, mods);
    }

    void GLFWCharCallback(GLFWwindow* window, unsigned int codepoint) {
        auto display = reinterpret_cast<DisplayGLFW*>(
            glfwGetWindowUserPointer(window));
        display->OnCharEvent(window, codepoint);
    }

    void GLFWScrollCallback(GLFWwindow* window, 
        double xoffset, double yoffset) {
        auto display = reinterpret_cast<DisplayGLFW*>(
            glfwGetWindowUserPointer(window));
        display->OnScrollEvent(window, xoffset, yoffset);
    }

    void GLFWMouseButtonCallback(GLFWwindow* window,
        int button, int action, int mods) {
        auto display = reinterpret_cast<DisplayGLFW*>(
            glfwGetWindowUserPointer(window));
        display->OnMouseButtonEvent(window, button, action, mods);
    }

    void GLFWCursorEnterCallback(GLFWwindow* window, int entered) {
        auto display = reinterpret_cast<DisplayGLFW*>(
            glfwGetWindowUserPointer(window));
        display->OnCursorEnterEvent(window, entered);
    }

    void GLFWCharModCallback(GLFWwindow* window,
        unsigned int codepoint, int mods) {
        auto display = reinterpret_cast<DisplayGLFW*>(
            glfwGetWindowUserPointer(window));
        display->OnCharModEvent(window, codepoint, mods);
    }

    void GLFWDropCallback(GLFWwindow* window, 
        int path_count, const char* paths[]) {
        auto display = reinterpret_cast<DisplayGLFW*>(
            glfwGetWindowUserPointer(window));
        display->OnDropEvent(window, path_count, paths);
    }

    void GLFWCursorPosCallback(GLFWwindow* window,
        double xpos, double ypos) {
        auto display = reinterpret_cast<DisplayGLFW*>(
            glfwGetWindowUserPointer(window));
        display->OnCursorPosEvent(window, xpos, ypos);
    }

    DisplayGLFW::DisplayGLFW(const RealtimeGraphicsParams& params) :
        mParams(params), mWindowPollEvent(marl::Event::Mode::Manual) {
    }

    void DisplayGLFW::Startup(const RealtimeGraphicsParams& params) {
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize glfw!");
        }

        glfwSetErrorCallback(&HandleGLFWError);

        int glfwApiHint = GLFW_NO_API;

        if (params.mDeviceType == GraphicsBackend::OPENGL) {
            glfwApiHint = GLFW_OPENGL_API;
        }

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

        if (mParams.mDeviceType == GraphicsBackend::OPENGL) {
			glfwMakeContextCurrent(mWindow);
        }

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
    }

    void DisplayGLFW::Startup(marl::WaitGroup& waitGroup) {
        Startup(mParams);
    }

    void DisplayGLFW::RegisterInterfaces(core::InterfaceCollection& interfaces) {
        interfaces.Add<IDisplay>(this);
        interfaces.Add<INativeWindowProvider>(this);
        interfaces.Add<IGLFWWindowProvider>(this);
    }

    bool DisplayGLFW::ShouldClose() const {
        return glfwWindowShouldClose(mWindow);
    }

    void DisplayGLFW::Shutdown() {
        glfwDestroyWindow(mWindow);
        glfwTerminate();
    }

    void DisplayGLFW::SetFrame(core::Frame& frame) {
    }

    void DisplayGLFW::LoadResources(marl::WaitGroup& waitGroup) {
    }

    void DisplayGLFW::RequestSync(core::SyncObject& syncObject) {
        mWindowPollEvent.clear();
    }

    void DisplayGLFW::Fork(core::Frame& frame, 
        core::SyncObject& syncObject,
        const core::Time& time) {
        
        if (bResizeRequested) {
            glfwSetWindowSize(mWindow, mResize.x, mResize.y);
            bResizeRequested = false;
        }

        UpdateCaptures();
        glfwPollEvents();
        UpdateCaptures();

        mWindowPollEvent.signal();
    }

    void DisplayGLFW::Join(core::Frame& frame) {
    }

    void DisplayGLFW::Wait() {
    }

    void DisplayGLFW::SetFramebufferSize(uint width, uint height) {
        bResizeRequested = true;
        mResize.x = width;
        mResize.y = height;
    }

    GLFWwindow* DisplayGLFW::GetWindowGLFW() const {
        return mWindow;
    }

    NativeWindow DisplayGLFW::GetWindow() {
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

    void DisplayGLFW::GLMakeContextCurrent() {
        if (mParams.mDeviceType == GraphicsBackend::OPENGL) {
            glfwMakeContextCurrent(mWindow);
        }
    }

    void DisplayGLFW::GLSwapBuffers(int swapInterval) {
        if (mParams.mDeviceType == GraphicsBackend::OPENGL) {
            
            if (mGLSwapInterval != swapInterval) {
                mGLSwapInterval = swapInterval;
                glfwSwapInterval(mGLSwapInterval);
            }

            glfwSwapBuffers(mWindow);
        }
    }

    glm::i32vec2 DisplayGLFW::GetFramebufferSize() const {
        glm::i32vec2 size;
        glfwGetFramebufferSize(mWindow, &size.x, &size.y);
        return size;
    }

    bool DisplayGLFW::GetIsFullscreen() const {
        return mParams.bFullscreen;
    }

    GraphicsBackend DisplayGLFW::GetRequestedBackend() const {
        return mParams.mDeviceType;
    }

    void DisplayGLFW::AddCapture(
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

    void DisplayGLFW::RemoveCapture(IInputCapture* capture) {
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

    void DisplayGLFW::UpdateCaptures() {
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
        DisplayGLFW::AddMouseButtonCallback(
            const mouse_button_callback_t& c, 
            CallbackPriority p,
            IInputCapture* capture) {
        AddCapture(capture, p);
        return mMouseButtonEvent.Add(c, capture, (double)p);
    }
    core::delegate_handle_t 
        DisplayGLFW::AddKeyCallback(
            const key_callback_t& c, 
            CallbackPriority p,
            IInputCapture* capture) {
        AddCapture(capture, p);
        return mKeyEvent.Add(c, capture, (double)p);
    }
    core::delegate_handle_t 
        DisplayGLFW::AddCharCallback(
            const char_callback_t& c, 
            CallbackPriority p,
            IInputCapture* capture) {
        AddCapture(capture, p);
        return mCharEvent.Add(c, capture, (double)p);
    }
    core::delegate_handle_t 
        DisplayGLFW::AddScrollCallback(
            const scroll_callback_t& c, 
            CallbackPriority p,
            IInputCapture* capture) {
        AddCapture(capture, p);
        return mScrollEvent.Add(c, capture, (double)p);
    }
    core::delegate_handle_t
        DisplayGLFW::AddCharModCallback(
            const char_mod_callback_t& c, 
            CallbackPriority p,
            IInputCapture* capture) {
        AddCapture(capture, p);
        return mCharModEvent.Add(c, capture, (double)p);
    }
    core::delegate_handle_t
        DisplayGLFW::AddCursorPosCallback(
            const cursor_pos_callback_t& c, 
            CallbackPriority p,
            IInputCapture* capture) {
        AddCapture(capture, p);
        return mCursorPosEvent.Add(c, capture, (double)p);
    }
    core::delegate_handle_t
        DisplayGLFW::AddDropCallback(
            const drop_callback_t& c, 
            CallbackPriority p) {
        return mDropEvent.Add(c, (double)p);
    }
    core::delegate_handle_t
        DisplayGLFW::AddCursorEnterCallback(
            const cursor_enter_callback_t& c, 
            CallbackPriority p,
            IInputCapture* capture) {
        AddCapture(capture, p);
        return mCursorEnterEvent.Add(c, capture, (double)p);
    }
        
    void DisplayGLFW::RemoveMouseButtonCallback(core::delegate_handle_t h) {
        RemoveCapture(mMouseButtonEvent.Remove(h));
    }
    void DisplayGLFW::RemoveKeyCallback(core::delegate_handle_t h) {
        RemoveCapture(mKeyEvent.Remove(h));
    }
    void DisplayGLFW::RemoveCharCallback(core::delegate_handle_t h) {
        RemoveCapture(mCharEvent.Remove(h));
    }
    void DisplayGLFW::RemoveScrollCallback(core::delegate_handle_t h) {
        RemoveCapture(mScrollEvent.Remove(h));
    }
    void DisplayGLFW::RemoveCharModCallback(core::delegate_handle_t h) {
        RemoveCapture(mCharModEvent.Remove(h));
    }
    void DisplayGLFW::RemoveCursorPosCallback(core::delegate_handle_t h) {
        RemoveCapture(mCursorPosEvent.Remove(h));
    }
    void DisplayGLFW::RemoveDropCallback(core::delegate_handle_t h) {
        mDropEvent.Remove(h);
    }
    void DisplayGLFW::RemoveCursorEnterCallback(core::delegate_handle_t h) {
        RemoveCapture(mCursorEnterEvent.Remove(h));
    }

    void DisplayGLFW::WaitForInput() {
        mWindowPollEvent.wait();
    }

    IInputCapture* DisplayGLFW::GetMouseFocus() {
        return mMouseFocus;
    }

    IInputCapture* DisplayGLFW::GetKeyboardFocus() {
        return mKeyboardFocus;
    }

    template <typename... Args>
    void DisplayGLFW::InvokeEvent(IInputCapture*& captureGroup,
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

    void DisplayGLFW::OnKeyEvent(GLFWwindow* window, 
        int key, int scancode, int action, int mods) {
        InvokeEvent(mKeyboardFocus, mKeyEvent, 
            window, key, scancode, action, mods);
    }
    void DisplayGLFW::OnCharEvent(GLFWwindow* window, 
        unsigned int codepoint) {
        InvokeEvent(mKeyboardFocus, mCharEvent, 
            window, codepoint);
    }
    void DisplayGLFW::OnScrollEvent(GLFWwindow* window, 
        double xoffset, double yoffset) {
        InvokeEvent(mMouseFocus, mScrollEvent, 
            window, xoffset, yoffset);
    }
    void DisplayGLFW::OnMouseButtonEvent(GLFWwindow* window, 
        int button, int action, int mods) {
        InvokeEvent(mMouseFocus, mMouseButtonEvent, 
            window, button, action, mods);
    }
    void DisplayGLFW::OnCharModEvent(GLFWwindow* window,
        unsigned int codepoint, int mods) {
        InvokeEvent(mKeyboardFocus, mCharModEvent,
            window, codepoint, mods);
    }
    void DisplayGLFW::OnDropEvent(GLFWwindow* window,
        int path_count, const char* paths[]) {
        mDropEvent(window, path_count, paths);
    }
    void DisplayGLFW::OnCursorPosEvent(GLFWwindow* window, 
        double xpos, double ypos) {
        InvokeEvent(mMouseFocus, mCursorPosEvent,
            window, xpos, ypos);
    }
    void DisplayGLFW::OnCursorEnterEvent(GLFWwindow* window, 
        int entered) {
        InvokeEvent(mMouseFocus, mCursorEnterEvent,
            window, entered);
    }
}

#endif