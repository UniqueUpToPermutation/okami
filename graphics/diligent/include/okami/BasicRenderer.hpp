#pragma once

#include <okami/System.hpp>
#include <okami/ResourceManager.hpp>
#include <okami/Geometry.hpp>
#include <okami/Graphics.hpp>
#include <okami/Display.hpp>

#include <RenderDevice.h>
#include <SwapChain.h>
#include <DeviceContext.h>
#include <RefCntAutoPtr.hpp>

namespace DG = Diligent;

namespace okami::graphics {
    class BasicRenderer : 
        public core::ISystem,
        public core::IResourceManager<core::Geometry> {
    private:
        DG::RefCntAutoPtr<DG::IRenderDevice>    mDevice;
        DG::RefCntAutoPtr<DG::IEngineFactory>   mEngineFactory;
        std::vector<DG::RefCntAutoPtr<DG::IDeviceContext>>   
            mContexts;
        DG::RefCntAutoPtr<DG::ISwapChain>       mSwapChain;
        core::ISystem*                          mDisplaySystem;
        INativeWindowProvider*                  mNativeWindowProvider;
        IDisplay*                               mDisplay;
        GraphicsBackend                         mBackend;

        core::ResourceManager<core::Geometry>   mGeometryManager;

    public:
        void OnDestroy(core::Geometry* geometry);
        void OnFinalize(core::Geometry* geometry);

        BasicRenderer(core::ISystem* displaySystem);

        void Startup(marl::WaitGroup& waitGroup) override;
        void Shutdown() override;

        void RegisterInterfaces(core::InterfaceCollection& interfaces) override;
        void LoadResources(core::Frame* frame, 
            marl::WaitGroup& waitGroup) override;
        void RequestSync(core::SyncObject& syncObject) override;
        void BeginExecute(core::Frame* frame, 
            marl::WaitGroup& renderGroup, 
            marl::WaitGroup& updateGroup,
            core::SyncObject& syncObject,
            const core::Time& time) override;
        void EndExecute(core::Frame* frame) override;

        core::Future<core::Handle<core::Geometry>> Load(
            const std::filesystem::path& path, 
            const core::LoadParams<core::Geometry>& params, 
            core::resource_id_t newResId) override;

        core::Future<core::Handle<core::Geometry>> Add(core::Geometry&& obj, 
            core::resource_id_t newResId) override;
        core::Future<core::Handle<core::Geometry>> Add(core::Geometry&& obj, 
            const std::filesystem::path& path, 
            core::resource_id_t newResId) override;
    };
}