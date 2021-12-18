#include <okami/Display.hpp>

#if USE_GLFW

#include <GLFW/glfw3native.h>

namespace okami::graphics {
    void DisplayGLFW::Startup(const RealtimeGraphicsParams& params) {
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize glfw!");
        }

        int glfwApiHint = GLFW_NO_API;

        if (params.mDeviceType == GraphicsBackend::OPENGL) {
            glfwApiHint = GLFW_OPENGL_API;
        }

        glfwWindowHint(GLFW_CLIENT_API, glfwApiHint);
        if (glfwApiHint == GLFW_OPENGL_API)
        {
            // We need compute shaders, so request OpenGL 4.2 at least
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
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

        if (mParams.mDeviceType == GraphicsBackend::OPENGL)
			glfwMakeContextCurrent(mWindow);
    }

    void DisplayGLFW::Startup(marl::WaitGroup& waitGroup) {
        Startup(mParams);
    }

    void DisplayGLFW::RegisterInterfaces(core::InterfaceCollection& interfaces) {
        interfaces.Add<IDisplay>(this);
        interfaces.Add<INativeWindowProvider>(this);
    }

    bool DisplayGLFW::ShouldClose() const {
        return glfwWindowShouldClose(mWindow);
    }

    void DisplayGLFW::Shutdown() {
        glfwDestroyWindow(mWindow);
        glfwTerminate();
    }

    void DisplayGLFW::LoadResources(core::Frame* frame, 
        marl::WaitGroup& waitGroup) {
    }

    void DisplayGLFW::BeginExecute(core::Frame* frame, 
        marl::WaitGroup& renderGroup, 
        marl::WaitGroup& updateGroup,
        core::SyncObject& syncObject,
        const core::Time& time) {
        glfwPollEvents();
    }

    void DisplayGLFW::EndExecute(core::Frame* frame) {
    }

    DisplayGLFW::DisplayGLFW(const RealtimeGraphicsParams& params) :
        mParams(params) {
    }

    std::unique_ptr<core::ISystem> CreateGLFWDisplay(const RealtimeGraphicsParams& params) {
        return std::make_unique<DisplayGLFW>(params);
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
		Window.pNSView = GetNSWindowView(window);
	#endif

        return Window;
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