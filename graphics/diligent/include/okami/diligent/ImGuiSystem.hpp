#pragma once

#include <okami/System.hpp>
#include <okami/Graphics.hpp>
#include <okami/diligent/Display.hpp>
#include <okami/diligent/RenderModule.hpp>
#include <okami/Event.hpp>

#include <RefCntAutoPtr.hpp>
#include <ImGuiImplDiligent.hpp>

namespace okami::graphics::diligent {

    class ImGuiRenderOverlay final :
        public IRenderModule {
    public:
        DG::SURFACE_TRANSFORM mSurfaceTransform;
        std::unique_ptr<DG::ImGuiImplDiligent> mImGuiImpl;
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

    class ImGuiSystem final : 
        public core::ISystem,
        public IImGuiCallback {
    private:
        ImGuiRenderOverlay mOverlay;
        IRenderer* mRenderer;
        core::ISystem* mInputSystem;
        core::Event<> mOnUpdate;
        marl::WaitGroup mUpdateWaitGroup;

    public:
        ImGuiSystem(IRenderer* renderer, core::ISystem* input);
        ~ImGuiSystem();

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