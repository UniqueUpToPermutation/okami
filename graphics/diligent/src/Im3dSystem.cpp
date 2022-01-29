#include <okami/diligent/Im3dSystem.hpp>

#include <iostream>

// Load shader locations in TEXT into a lookup
void MakeShaderMap(okami::core::file_map_t* map);

using namespace okami::core;

namespace okami::graphics::diligent {

    Im3d::Vec3 ToIm3d(const DG::float3& f) {
        return Im3d::Vec3(f.x, f.y, f.z);
    }

    Im3d::Vec2 ToIm3d(const DG::float2& f) {
        return Im3d::Vec2(f.x, f.y);
    }

    Im3d::Mat4 ToIm3d(const DG::float4x4& f) {
        return Im3d::Mat4(
            f.m00, f.m01, f.m02, f.m03,
            f.m10, f.m11, f.m12, f.m13,
            f.m20, f.m21, f.m22, f.m23,
            f.m30, f.m31, f.m32, f.m33);
    }

    void Im3dRenderOverlay::Startup(core::ISystem* renderer,
        DG::IRenderDevice* device,
        const RenderModuleParams& params) {

        core::InterfaceCollection interfaces;
        renderer->RegisterInterfaces(interfaces);

        auto globalsBuffer = interfaces.Query<IGlobalsBufferProvider>();
        auto rendererInterface = interfaces.Query<IRenderer>();

        if (!globalsBuffer) {
            throw std::runtime_error(
                "Renderer does not implement IGlobalsBufferProvider!");
        }

        if (!rendererInterface) {
            throw std::runtime_error(
                "Renderer does not implement IRenderer!");
        }

        mShaders = Im3dShaders::LoadDefault(device, params.mFileSystem);
        
        mPipeline = Im3dPipeline(device, 
            *globalsBuffer->GetGlobalsBuffer(),
            ToDiligent(rendererInterface->GetFormat(RenderAttribute::COLOR)),
            ToDiligent(rendererInterface->GetDepthFormat(RenderPass::Final())),
            1,
            mShaders,
            true);

        mPipelineNoDepth = Im3dPipeline(device,
            *globalsBuffer->GetGlobalsBuffer(),
            ToDiligent(rendererInterface->GetFormat(RenderAttribute::COLOR)),
            ToDiligent(rendererInterface->GetDepthFormat(RenderPass::Final())),
            1,
            mShaders,
            false);

        mModule = Im3dModule(device);
    }

    void Im3dRenderOverlay::QueueCommands(
        DG::IDeviceContext* context,
        const core::Frame& frame,
        const RenderView& view,
        const RenderPass& pass,
        const RenderModuleGlobals& globals) {
        assert(pass.IsFinal());

        auto target = view.mTarget.Ptr();
        auto it = mCanvasInfos.find(target);
        
        if (it != mCanvasInfos.end()) {
            mRenderReady.wait();

            mModule.Draw(context, mPipeline, 
                &it->second->mContext);
            mModule.Draw(context, mPipelineNoDepth, 
                &it->second->mContextNoDepth);

            it->second->mLastFrameGlobals = globals;
        }
    }

    void Im3dRenderOverlay::Shutdown() {
        mModule = Im3dModule();
        mPipeline = Im3dPipeline();
        mPipelineNoDepth = Im3dPipeline();
        mShaders = Im3dShaders();
    }

    bool Im3dRenderOverlay::CanvasImpl::ShouldCaptureMouse() const {
        return mContext.m_appActiveId == Im3d::Id_Invalid;
    }

    bool Im3dRenderOverlay::CanvasImpl::ShouldCaptureKeyboard() const {
        return false;
    }

    Im3dRenderOverlay::CanvasImpl::~CanvasImpl() {
        if (mInput) {
            mInput->RemoveKeyCallback(
                mKeyHandler);
            mInput->RemoveMouseButtonCallback(
                mMouseButtonHandler);
            mInput->RemoveCursorPosCallback(
                mMousePosHandler);
        }
    }

    void Im3dRenderOverlay::AddCanvas(
        RenderCanvas* canvas, 
        core::IInputProvider* input,
        double scale) {

        if (mCanvasInfos.find(canvas) != mCanvasInfos.end()) {
            throw std::runtime_error("Overlay has already been attached to canvas!");
        }
        
        auto info = std::make_unique<CanvasImpl>();

        info->mCanvas = canvas;
        info->mInput = input;

        info->mKeyHandler = input->AddKeyCallback(
            [&appData = info->mAppData](
                IInputProvider* inputProvider,
                Key key, 
                int scancode, 
                KeyAction action, 
                KeyModifiers mods) {
            
            if (action == KeyAction::PRESS) {
                switch (key) {
                case Key::L:
                    appData.m_keyDown[Im3d::Key_L] = true;
                    break;
                case Key::S:
                    appData.m_keyDown[Im3d::Key_S] = true;
                    break;
                case Key::T:
                    appData.m_keyDown[Im3d::Key_T] = true;
                    break;
                case Key::R:
                    appData.m_keyDown[Im3d::Key_R] = true;
                    break;
                default:
                    break;
                }
            } else if (action == KeyAction::RELEASE) {
                switch (key) {
                case Key::L:
                    appData.m_keyDown[Im3d::Key_L] = false;
                    break;
                case Key::S:
                    appData.m_keyDown[Im3d::Key_S] = false;
                    break;
                case Key::T:
                    appData.m_keyDown[Im3d::Key_T] = false;
                    break;
                case Key::R:
                    appData.m_keyDown[Im3d::Key_R] = false;
                    break;
                default:
                    break;
                }
            }

            return false;
        }, CallbackPriority::MEDIUM, info.get());

        info->mMousePosHandler = input->AddCursorPosCallback(
            [&input = info->mInputCache]
                (IInputProvider* inputProvider,
                    double xpos, double ypos) {

            input.mMousePosX = xpos;
            input.mMousePosY = ypos;

            return false;
        }, CallbackPriority::HIGH, info.get());

        info->mMouseButtonHandler = input->AddMouseButtonCallback(
            [&appData = info->mAppData]
                (IInputProvider* inputProvider,
                MouseButton button, 
                KeyAction action, 
                KeyModifiers mods) {

            if (button == MouseButton::LEFT) {
                if (action == KeyAction::PRESS) {
                    appData.m_keyDown[Im3d::Mouse_Left] = true;
                } else if (action == KeyAction::RELEASE) {
                    appData.m_keyDown[Im3d::Mouse_Left] = false;
                }
            }

            return false;
        }, CallbackPriority::HIGH, info.get());

        mCanvasInfos.emplace(canvas, std::move(info));
    }

    core::delegate_handle_t Im3dRenderOverlay::AddCallback(
        RenderCanvas* canvas,
        core::IInputProvider* input,
        immedate_callback_t callback) {

        auto it = mCanvasInfos.find(canvas);

        if (it == mCanvasInfos.end()) {
            AddCanvas(canvas, input, 2.0);
            canvas->AddOverlay(this);
        }

        return mOnUpdate.Add(canvas, std::move(callback));
    }

    core::delegate_handle_t Im3dRenderOverlay::AddNoDepthCallback(
        RenderCanvas* canvas,
        core::IInputProvider* input,
        immedate_callback_t callback) {

        auto it = mCanvasInfos.find(canvas);

        if (it == mCanvasInfos.end()) {
            AddCanvas(canvas, input, 2.0);
            canvas->AddOverlay(this);
        }

        return mOnUpdateNoDepth.Add(canvas, std::move(callback));
    }

    void Im3dRenderOverlay::RemoveCanvas(RenderCanvas* canvas) {
        auto it = mCanvasInfos.find(canvas);

        if (it != mCanvasInfos.end()) {
            mCanvasInfos.erase(it);
        } else {
            throw std::runtime_error("Tried to dettach when not attached!");
        }

        mOnUpdate.RemoveAll(canvas);
        mOnUpdateNoDepth.RemoveAll(canvas);
    }

    void Im3dRenderOverlay::OnAttach(RenderCanvas* canvas) {
    }

    void Im3dRenderOverlay::OnDettach(RenderCanvas* canvas) {
        RemoveCanvas(canvas);
    }

    void Im3dRenderOverlay::Update(bool bAllowBlock) {
    }

    void Im3dRenderOverlay::WaitUntilReady(core::SyncObject& obj) {
        mRenderReady.wait();
    }

    Im3dRenderOverlay::Im3dRenderOverlay() :
        mRenderReady(marl::Event::Mode::Manual) {
    }

    void Im3dRenderOverlay::UpdateAllCanvases() {
        for (auto& [key, canvas] : mCanvasInfos) {
            UpdateCanvas(*canvas);
        }
    }

    void Im3dRenderOverlay::UpdateCanvas(CanvasImpl& canvas) {

        auto& appData = canvas.mAppData;
        auto& globals = canvas.mLastFrameGlobals;
        auto input = canvas.mInput;
        auto& inputCache = canvas.mInputCache;

        appData.m_deltaTime = globals.mTime.mTimeElapsed;
        appData.m_viewportSize = ToIm3d(globals.mViewportSize);
        appData.m_viewOrigin = ToIm3d(globals.mViewOrigin);
        appData.m_viewDirection = ToIm3d(globals.mViewDirection);
        appData.m_worldUp = ToIm3d(globals.mWorldUp);
        appData.m_projOrtho = globals.mCamera.mType == core::Camera::Type::ORTHOGRAPHIC;
    
        auto viewProj = globals.mView * globals.mProjection;
        auto viewProjInv = viewProj.Inverse();

        appData.setCullFrustum(ToIm3d(viewProj), true);

        float scale = 1.0;
        
        if (input) {
            input->WaitForInput();
            // World space cursor ray from mouse position; for VR this might be the position/orientation of the HMD or a tracked controller.
            DG::float2 cursorPos(inputCache.mMousePosX, inputCache.mMousePosY);
            cursorPos = 2.0f * cursorPos / globals.mViewportSize - DG::float2(1.0f, 1.0f);
            cursorPos.y = -cursorPos.y; // window origin is top-left, ndc is bottom-left

            DG::float4 rayDir = DG::float4(cursorPos.x, cursorPos.y, -1.0f, 1.0f) * viewProjInv; 
            rayDir = rayDir / rayDir.w;
            DG::float3 rayDirection = DG::float3(rayDir.x, rayDir.y, rayDir.z) - globals.mViewOrigin;
            rayDirection = DG::normalize(rayDirection);

            appData.m_cursorRayOrigin = ToIm3d(globals.mViewOrigin);
            appData.m_cursorRayDirection = ToIm3d(rayDirection);
        }

        scale = 2.0;

        // m_projScaleY controls how gizmos are scaled in world space to maintain a constant screen height
        appData.m_projScaleY = appData.m_projOrtho 
            ? 2.0f / globals.mProjection.m11 :
            tanf(globals.mCamera.mFieldOfView * 0.5f) * 2.0f; // or vertical fov for a perspective projection
        appData.m_projScaleY *= scale;

        auto& defaultContext = Im3d::GetContext();

        Im3d::SetContext(canvas.mContext);
        Im3d::GetAppData() = appData;
        Im3d::NewFrame();            
        mOnUpdate.InvokeOnly(canvas.mCanvas);
        Im3d::EndFrame();

        Im3d::SetContext(canvas.mContextNoDepth);
        Im3d::GetAppData() = appData;
        Im3d::NewFrame();    
        mOnUpdateNoDepth.InvokeOnly(canvas.mCanvas);
        Im3d::EndFrame();

        Im3d::SetContext(defaultContext);
    }

    Im3dSystem::Im3dSystem(
        IRenderer* renderer) : 
        mRenderer(renderer) {
    }

    Im3dSystem::Im3dSystem(
        IRenderer* renderer, 
        IWindow* window) : 
            Im3dSystem(renderer, window, window->GetCanvas()) {
    }

    Im3dSystem::Im3dSystem(
        IRenderer* renderer,
        IInputProvider* input,
        RenderCanvas* canvas) {
        AttachTo(canvas, input);
    }

    Im3dSystem::~Im3dSystem() {
        mRenderer->RemoveOverlay(&mOverlay);
    }

    core::delegate_handle_t Im3dSystem::Add(
        RenderCanvas* canvas,
        core::IInputProvider* input,
        immedate_callback_t callback) {
        return mOverlay.AddCallback(canvas, input, std::move(callback));
    }

    core::delegate_handle_t 
        Im3dSystem::AddNoDepth(
            RenderCanvas* canvas,
            core::IInputProvider* input,
            immedate_callback_t callback) {
        return mOverlay.AddNoDepthCallback(canvas, input, std::move(callback));
    }

    void Im3dSystem::Remove(core::delegate_handle_t handle) {
        mOverlay.mOnUpdate.Remove(handle);
    }

    void Im3dSystem::DettachFrom(RenderCanvas* canvas) {
        canvas->RemoveOverlay(&mOverlay);
    }

    void Im3dSystem::Startup(marl::WaitGroup& waitGroup) {
    }
    void Im3dSystem::RegisterInterfaces(core::InterfaceCollection& interfaces) {
        interfaces.Add<IIm3dSystem>(this);
    }
    void Im3dSystem::Shutdown() {

    }
    void Im3dSystem::LoadResources(marl::WaitGroup& waitGroup) {
    }
    void Im3dSystem::SetFrame(core::Frame& frame) {
    }
    void Im3dSystem::RequestSync(core::SyncObject& syncObject) {
        mOverlay.mRenderReady.clear();
    }
    void Im3dSystem::Fork(core::Frame& frame, 
        core::SyncObject& syncObject,
        const core::Time& time) {
        marl::schedule([
            &overlay = mOverlay]() {
            overlay.UpdateAllCanvases();
        });
    }
    void Im3dSystem::Join(core::Frame& frame) {
        Wait();
    }
    void Im3dSystem::Wait() {
        mOverlay.mRenderReady.wait();
    }
}

namespace okami::graphics {
    std::unique_ptr<core::ISystem> CreateIm3d(
        IRenderer* renderer,
        core::IInputProvider* input,
        RenderCanvas* canvas) {

        return std::make_unique<diligent::Im3dSystem>(
            renderer, input, canvas);
    }

    std::unique_ptr<core::ISystem> CreateIm3d(
        IRenderer* renderer,
        IWindow* window) {

        return std::make_unique<diligent::Im3dSystem>(
            renderer, window);
    }

    std::unique_ptr<core::ISystem> CreateIm3d(
        IRenderer* renderer) {

        return std::make_unique<diligent::Im3dSystem>(
            renderer);
    }
}