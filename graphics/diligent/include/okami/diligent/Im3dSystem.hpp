#pragma once

#include <okami/System.hpp>
#include <okami/Graphics.hpp>
#include <okami/Event.hpp>
#include <okami/diligent/Display.hpp>
#include <okami/diligent/RenderModule.hpp>
#include <okami/diligent/Im3dModule.hpp>

#include <RefCntAutoPtr.hpp>
#include <im3d.h>

namespace okami::graphics::diligent {

    class Im3dRenderOverlay final :
        public IRenderModule {
    public:
        DG::SURFACE_TRANSFORM mSurfaceTransform;
        bool bDraw = false;
        
        Im3dShaders mShaders;
        Im3dPipeline mPipeline;
        Im3dModule mModule;
        marl::Event mRenderReady;
        marl::Event mRenderFinished;

        void Startup(
            core::ISystem* renderer,
            DG::IRenderDevice* device, 
            DG::ISwapChain* swapChain,
            const RenderModuleParams& params) override;
        void QueueCommands(
            DG::IDeviceContext* context, 
            RenderPass pass) override;
        void Shutdown() override;
    };

    class Im3dSystem final :
        public core::ISystem,
        public IIm3dCallback {
    private:
        Im3dRenderOverlay mOverlay;
        IRenderer* mRenderer;
        core::Event<> mOnUpdate;
        marl::WaitGroup mUpdateWaitGroup;

    public:
        Im3dSystem(IRenderer* renderer);
        ~Im3dSystem();

        core::delegate_handle_t Add(immedate_callback_t callback) override;
        void Remove(core::delegate_handle_t handle) override;
        marl::WaitGroup& GetUpdateWaitGroup() override;

        void Startup(marl::WaitGroup& waitGroup) override;
        void RegisterInterfaces(core::InterfaceCollection& interfaces) override;
        void Shutdown() override;
        void LoadResources(marl::WaitGroup& waitGroup) override;
        void SetFrame(core::Frame& frame) override;
        void RequestSync(core::SyncObject& syncObject) override;
        void Fork(core::Frame& frame, 
            core::SyncObject& syncObject,
            const core::Time& time) override;
        void Join(core::Frame& frame) override;
        void Wait() override;
    };
}