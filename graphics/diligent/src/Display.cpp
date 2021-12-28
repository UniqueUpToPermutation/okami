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
    }

    void DisplayGLFW::Fork(core::Frame& frame, 
        core::SyncObject& syncObject,
        const core::Time& time) {
        glfwPollEvents();

        if (bResizeRequested) {
            glfwSetWindowSize(mWindow, mResize.x, mResize.y);
            bResizeRequested = false;
        }
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

    DisplayGLFW::DisplayGLFW(const RealtimeGraphicsParams& params) :
        mParams(params) {
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
}

#endif