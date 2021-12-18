#pragma once

#include <okami/System.hpp>
#include <okami/ResourceManager.hpp>

#include <RenderDevice.h>
#include <SwapChain.h>
#include <DeviceContext.h>
#include <RefCntAutoPtr.hpp>

namespace DG = Diligent;

namespace okami::graphics {
    class BasicRenderer : public core::ISystem {
    private:
        DG::RefCntAutoPtr<DG::IRenderDevice>    mDevice;
        DG::RefCntAutoPtr<DG::IEngineFactory>   mEngineFactory;
        std::vector<DG::RefCntAutoPtr<DG::IDeviceContext>>   
            mContexts;
        DG::RefCntAutoPtr<DG::ISwapChain>       mSwapChain;
        core::ISystem*                          mDisplaySystem;

    public:
        BasicRenderer(core::ISystem* displaySystem);

        void Startup(marl::WaitGroup& waitGroup) override;
        void Shutdown() override;

        void RegisterInterfaces(core::InterfaceCollection& interfaces) override;
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