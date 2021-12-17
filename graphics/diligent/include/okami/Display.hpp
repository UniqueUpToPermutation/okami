#pragma once

#include <okami/Graphics.hpp>

#if USE_GLFW
#include <GLFW/glfw3.h>

namespace okami::graphics {
    class DisplayGLFW : public core::ISystem, public IWindow {
    private:
        GLFWwindow* mWindow = nullptr;
		bool bOwnsWindow;
		RealtimeGraphicsParams mParams;

        void Startup(const RealtimeGraphicsParams& params);

    public:
        DisplayGLFW(const RealtimeGraphicsParams& params);

        bool ShouldClose() const override;

        void RegisterInterfaces(core::InterfaceCollection& interfaces) override;
        void Startup(marl::WaitGroup& waitGroup) override;
        void Shutdown() override;
        void LoadResources(core::Frame* frame, 
            marl::WaitGroup& waitGroup) override;
        void BeginExecute(core::Frame* frame, 
            marl::WaitGroup& renderGroup, 
            marl::WaitGroup& updateGroup,
            core::SyncObject& syncObject,
            const core::Time& time) override;
        void EndExecute(core::Frame* frame) override;
    };
}

#endif