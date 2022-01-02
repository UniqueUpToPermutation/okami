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
    inline Im3d::Vec2 ToIm2d(const glm::vec2& v) {
        return Im3d::Vec2(v.x, v.y);
    }

    inline Im3d::Vec3 ToIm3d(const glm::vec3& v) {
        return Im3d::Vec3(v.x, v.y, v.z);
    }

    inline Im3d::Vec4 ToIm4d(const glm::vec4& v) {
        return Im3d::Vec4(v.x, v.y, v.z, v.w);
    }

    inline Im3d::Mat4 ToIm3d(const glm::mat4& m) {
        return Im3d::Mat4(
            m[0][0], m[1][0], m[2][0], m[3][0],
            m[0][1], m[1][1], m[2][1], m[3][1],
            m[0][2], m[1][2], m[2][2], m[3][2],
            m[0][3], m[1][3], m[2][3], m[3][3]);
    }

    class Im3dRenderOverlay final :
        public IRenderModule {
    public:
        DG::SURFACE_TRANSFORM mSurfaceTransform;
        bool bDraw = false;
        
        Im3dShaders mShaders;
        Im3dPipeline mPipeline;
        Im3dPipeline mPipelineNoDepth;
        Im3dModule mModule;
        marl::Event mRenderReady;
        marl::Event mRenderFinished;

        Im3d::Context* mContext;
        Im3d::Context* mContextNoDepth;

        RenderModuleGlobals mLastFrameGlobals;

        Im3dRenderOverlay(Im3d::Context* context,
            Im3d::Context* contextNoDepth);

        void Startup(
            core::ISystem* renderer,
            DG::IRenderDevice* device, 
            DG::ISwapChain* swapChain,
            const RenderModuleParams& params) override;
        void QueueCommands(
            DG::IDeviceContext* context, 
            RenderPass pass,
            const RenderModuleGlobals& globals) override;
        void Shutdown() override;
    };

    class Im3dSystem final :
        public core::ISystem,
        public IIm3dCallback,
        public IInputCapture {
    private:
        Im3dRenderOverlay mOverlay;
        IRenderer* mRenderer = nullptr;
        IGLFWWindowProvider* mInput = nullptr;
        core::Event<> mOnUpdate;
        core::Event<> mOnUpdateNoDepth;

        core::delegate_handle_t mKeyHandler;
        core::delegate_handle_t mMousePosHandler;
        core::delegate_handle_t mMouseButtonHandler;

        Im3d::Context mContext;
        Im3d::Context mContextNoDepth;
        Im3d::AppData mAppData;

        double mScale = 0.0;

        struct InputCache {
            double mMousePosX;
            double mMousePosY;
        } mInputCache;

    public:
        Im3dSystem(IRenderer* renderer);
        Im3dSystem(IRenderer* renderer, IGLFWWindowProvider* input);
        ~Im3dSystem();

        core::delegate_handle_t Add(immedate_callback_t callback) override;
        core::delegate_handle_t 
            AddNoDepth(immedate_callback_t callback) override;
        void Remove(core::delegate_handle_t handle) override;

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

        bool SupportsInput() const override;
        bool ShouldCaptureMouse() const override;
        bool ShouldCaptureKeyboard() const override;
    };
}