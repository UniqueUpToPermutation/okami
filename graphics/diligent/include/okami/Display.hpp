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

namespace DG = Diligent;

namespace okami::graphics {
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

namespace okami::graphics {

    class DisplayGLFW : 
        public core::ISystem, 
        public IDisplay,
        public INativeWindowProvider {
    private:
        GLFWwindow* mWindow = nullptr;
		RealtimeGraphicsParams mParams;
        int mGLSwapInterval = -1;

        void Startup(const RealtimeGraphicsParams& params);

    public:
        DisplayGLFW(const RealtimeGraphicsParams& params);

        bool ShouldClose() const override;

        void RegisterInterfaces(core::InterfaceCollection& interfaces) override;
        void Startup(marl::WaitGroup& waitGroup) override;
        void Shutdown() override;
        void LoadResources(core::Frame* frame, 
            marl::WaitGroup& waitGroup) override;
        void RequestSync(core::SyncObject& syncObject) override;
        void BeginExecute(core::Frame* frame, 
            marl::WaitGroup& renderGroup, 
            marl::WaitGroup& updateGroup,
            core::SyncObject& syncObject,
            const core::Time& time) override;
        void EndExecute(core::Frame* frame) override;

        NativeWindow GetWindow() override;
        void GLMakeContextCurrent() override;
        void GLSwapBuffers(int swapInterval) override;

        glm::i32vec2 GetFramebufferSize() const override;
        bool GetIsFullscreen() const override;
        GraphicsBackend GetRequestedBackend() const override;
    };
}

#endif