#pragma once

#include <okami/System.hpp>
#include <okami/Graphics.hpp>
#include <okami/Event.hpp>
#include <okami/Input.hpp>
#include <okami/diligent/RenderModule.hpp>
#include <okami/diligent/Im3dModule.hpp>
#include <okami/diligent/Im3dGizmo.hpp>

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
        public IOverlayModule {
    public:
        DG::SURFACE_TRANSFORM mSurfaceTransform;

        struct CanvasImpl final : public core::IInputCapture {
            RenderCanvas* mCanvas = nullptr;
            core::IInputProvider* mInput = nullptr;

            Im3d::Context mContext;
            Im3d::Context mContextNoDepth;

            core::delegate_handle_t mKeyHandler = 0;
            core::delegate_handle_t mMousePosHandler = 0;
            core::delegate_handle_t mMouseButtonHandler = 0;

            Im3d::AppData mAppData;
            RenderModuleGlobals mLastFrameGlobals;

            double mScale = 0.0;

            struct InputCache {
                double mMousePosX;
                double mMousePosY;
            } mInputCache;

            bool ShouldCaptureMouse() const override;
            bool ShouldCaptureKeyboard() const override;

            ~CanvasImpl();
        };

        core::UserDataEvent<RenderCanvas*> mOnUpdate;
        core::UserDataEvent<RenderCanvas*> mOnUpdateNoDepth;

        std::unordered_map<const RenderCanvas*,
            std::unique_ptr<CanvasImpl>> mCanvasInfos;

        Im3dShaders mShaders;
        Im3dPipeline mPipeline;
        Im3dPipeline mPipelineNoDepth;
        Im3dModule mModule;

        marl::Event mRenderReady;

        void UpdateCanvas(CanvasImpl& canvas);
        void UpdateAllCanvases();

        void Startup(
            core::ISystem* renderer,
            DG::IRenderDevice* device,
            const RenderModuleParams& params) override;
        void QueueCommands(
            DG::IDeviceContext* context,
            const core::Frame& frame,
            const RenderView& view,
            const RenderCanvas& target,
            const RenderPass& pass,
            const RenderModuleGlobals& globals) override;
        void Shutdown() override;
        void AddCanvas(
            RenderCanvas* canvas, 
            core::IInputProvider* input,
            double scale);
        void RemoveCanvas(RenderCanvas* canvas);

        core::delegate_handle_t AddCallback(
            RenderCanvas* canvas,
            core::IInputProvider* input,
            immedate_callback_t callback);
        core::delegate_handle_t AddNoDepthCallback(
            RenderCanvas* canvas,
            core::IInputProvider* input, 
            immedate_callback_t callback);

        void OnAttach(RenderCanvas* canvas) override;
        void OnDettach(RenderCanvas* canvas) override;

        void DettachAll();

        void Update(core::ResourceManager*) override;
        void WaitUntilReady(core::SyncObject& obj) override;

        bool IsIdle() override;
        void WaitOnPendingTasks() override;

        Im3dRenderOverlay();
    };

    class Im3dSystem final :
        public core::ISystem,
        public IIm3dSystem {
    private:
        Im3dRenderOverlay mOverlay;
        Im3dGizmo mGizmo;
        IRenderer* mRenderer = nullptr;
        double mScale = 1.0;

    public:
        Im3dSystem(IRenderer* renderer);
        Im3dSystem(IRenderer* renderer, 
            IWindow* window);
        Im3dSystem(IRenderer* renderer,
            core::IInputProvider* input,
            RenderCanvas* canvas);
        ~Im3dSystem();

        core::delegate_handle_t Add(
            RenderCanvas* canvas, 
            core::IInputProvider* input,
            immedate_callback_t callback) override;
        core::delegate_handle_t AddNoDepth(
            RenderCanvas* canvas,
            core::IInputProvider* input,
            immedate_callback_t callback) override;

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

        void AttachTo(RenderCanvas* canvas, core::IInputProvider* provider);
        void DettachFrom(RenderCanvas* canvas);
    };
}