#include <okami/diligent/Im3dSystem.hpp>

#include <iostream>

// Load shader locations in TEXT into a lookup
void MakeShaderMap(okami::core::file_map_t* map);

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

    Im3dRenderOverlay::Im3dRenderOverlay(Im3d::Context* context,
        Im3d::Context* contextNoDepth) :
        mContext(context),
        mContextNoDepth(contextNoDepth) {
    }

    void Im3dRenderOverlay::Startup(core::ISystem* renderer, 
        DG::IRenderDevice* device, 
        DG::ISwapChain* swapChain,
        const RenderModuleParams& params) {

        core::InterfaceCollection interfaces;
        renderer->RegisterInterfaces(interfaces);

        auto globalsBuffer = interfaces.Query<IGlobalsBufferProvider>();

        if (!globalsBuffer) {
            throw std::runtime_error(
                "Renderer does not implement IGlobalsBufferProvider!");
        }

        // Load shaders
        core::EmbeddedFileLoader fileLoader(&MakeShaderMap);
        
        mShaders = Im3dShaders::LoadDefault(device, &fileLoader);
        mPipeline = Im3dPipeline(device, 
            *globalsBuffer->GetGlobalsBuffer(),
            swapChain->GetCurrentBackBufferRTV()->GetDesc().Format,
            swapChain->GetDepthBufferDSV()->GetDesc().Format,
            1,
            mShaders,
            true);

        mPipelineNoDepth = Im3dPipeline(device,
            *globalsBuffer->GetGlobalsBuffer(),
            swapChain->GetCurrentBackBufferRTV()->GetDesc().Format,
            swapChain->GetDepthBufferDSV()->GetDesc().Format,
            1,
            mShaders,
            false);

        mModule = Im3dModule(device);
    }


    void Im3dRenderOverlay::QueueCommands(
        DG::IDeviceContext* context, 
        RenderPass pass,
        const RenderModuleGlobals& globals) {
        assert(pass == RenderPass::OVERLAY);

        mRenderReady.wait();
        mModule.Draw(context, mPipeline, mContext);
        mModule.Draw(context, mPipelineNoDepth, mContextNoDepth);
        mLastFrameGlobals = globals;
        mRenderFinished.signal();
    }

    void Im3dRenderOverlay::Shutdown() {
        mModule = Im3dModule();
        mPipeline = Im3dPipeline();
        mPipelineNoDepth = Im3dPipeline();
        mShaders = Im3dShaders();
    }

    Im3dSystem::Im3dSystem(IRenderer* renderer) :
        mRenderer(renderer),
        mOverlay(&mContext, &mContextNoDepth) {   
        mRenderer->AddOverlay(&mOverlay);
    }
    Im3dSystem::Im3dSystem(IRenderer* renderer, 
        IGLFWWindowProvider* glfw) :
        Im3dSystem(renderer) {
        mInput = glfw;
    }

    Im3dSystem::~Im3dSystem() {
        mRenderer->RemoveOverlay(&mOverlay);
    }

    core::delegate_handle_t Im3dSystem::Add(immedate_callback_t callback) {
        return mOnUpdate.Add(std::move(callback));
    }
    core::delegate_handle_t 
        Im3dSystem::AddNoDepth(immedate_callback_t callback) {
        return mOnUpdateNoDepth.Add(std::move(callback));
    }

    void Im3dSystem::Remove(core::delegate_handle_t handle) {
        mOnUpdate.Remove(handle);
    }

    void Im3dSystem::Startup(marl::WaitGroup& waitGroup) {
        if (mInput) {

            mMousePosHandler = mInput->AddCursorPosCallback(
                [&input = mInputCache](GLFWwindow* window, double xpos, double ypos) {

                input.mMousePosX = xpos;
                input.mMousePosY = ypos;

                return false;
            }, CallbackPriority::HIGH, this);

            mMouseButtonHandler = mInput->AddMouseButtonCallback(
                [&appData = mAppData](GLFWwindow* window, int button, int action, int mods) {

                if (button == GLFW_MOUSE_BUTTON_LEFT) {
                    if (action == GLFW_PRESS) {
                        appData.m_keyDown[Im3d::Mouse_Left] = true;
                    } else if (action == GLFW_RELEASE) {
                        appData.m_keyDown[Im3d::Mouse_Left] = false;
                    }
                }

                return false;
            }, CallbackPriority::HIGH, this);

            mKeyHandler = mInput->AddKeyCallback(
                [&appData = mAppData](GLFWwindow* window, 
                    int key, int scancode, int action, int mods) {
                
                if (action == GLFW_PRESS) {
                    switch (key) {
                    case GLFW_KEY_L:
                        appData.m_keyDown[Im3d::Key_L] = true;
                        break;
                    case GLFW_KEY_S:
                        appData.m_keyDown[Im3d::Key_S] = true;
                        break;
                    case GLFW_KEY_T:
                        appData.m_keyDown[Im3d::Key_T] = true;
                        break;
                    case GLFW_KEY_R:
                        appData.m_keyDown[Im3d::Key_R] = true;
                        break;
                    default:
                        break;
                    }
                } else if (action == GLFW_RELEASE) {
                    switch (key) {
                    case GLFW_KEY_L:
                        appData.m_keyDown[Im3d::Key_L] = false;
                        break;
                    case GLFW_KEY_S:
                        appData.m_keyDown[Im3d::Key_S] = false;
                        break;
                    case GLFW_KEY_T:
                        appData.m_keyDown[Im3d::Key_T] = false;
                        break;
                    case GLFW_KEY_R:
                        appData.m_keyDown[Im3d::Key_R] = false;
                        break;
                    default:
                        break;
                    }
                }

                return false;
            }, CallbackPriority::MEDIUM, this);
        }
    }
    void Im3dSystem::RegisterInterfaces(core::InterfaceCollection& interfaces) {
        interfaces.Add<IIm3dCallback>(this);
    }
    void Im3dSystem::Shutdown() {
        if (mInput) {
            mInput->RemoveCursorPosCallback(mMousePosHandler);
            mInput->RemoveMouseButtonCallback(mMouseButtonHandler);
            mInput->RemoveKeyCallback(mKeyHandler);
        }
    }
    void Im3dSystem::LoadResources(marl::WaitGroup& waitGroup) {
    }
    void Im3dSystem::SetFrame(core::Frame& frame) {
    }
    void Im3dSystem::RequestSync(core::SyncObject& syncObject) {
    }
    void Im3dSystem::Fork(core::Frame& frame, 
        core::SyncObject& syncObject,
        const core::Time& time) {
        marl::schedule([
            renderer = mRenderer,
            &update = mOnUpdate,
            &updateNoDepth = mOnUpdateNoDepth,
            &overlay = mOverlay,
            input = mInput,
            &inputCache = mInputCache,
            &context = mContext,
            &contextNoDepth = mContextNoDepth,
            &appData = mAppData]() {
            defer(overlay.mRenderReady.signal());

            auto& globals = overlay.mLastFrameGlobals;

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

                scale = input->GetContentScale();
            }

            // m_projScaleY controls how gizmos are scaled in world space to maintain a constant screen height
            appData.m_projScaleY = appData.m_projOrtho 
                ? 2.0f / globals.mProjection.m11 :
                tanf(globals.mCamera.mFieldOfView * 0.5f) * 2.0f; // or vertical fov for a perspective projection
            appData.m_projScaleY *= scale;

            auto& defaultContext = Im3d::GetContext();

            Im3d::SetContext(context);
            Im3d::GetAppData() = appData;
            Im3d::NewFrame();            
            update();
            Im3d::EndFrame();

            Im3d::SetContext(contextNoDepth);
            Im3d::GetAppData() = appData;
            Im3d::NewFrame();    
            updateNoDepth();
            Im3d::EndFrame();

            Im3d::SetContext(defaultContext);
        });
    }
    void Im3dSystem::Join(core::Frame& frame) {
        Wait();
    }
    void Im3dSystem::Wait() {
        mOverlay.mRenderFinished.wait();
    }
    bool Im3dSystem::SupportsInput() const {
        return mInput != nullptr;
    }
    bool Im3dSystem::ShouldCaptureMouse() const {
        return Im3d::GetActiveId() != Im3d::Id_Invalid;
    }
    bool Im3dSystem::ShouldCaptureKeyboard() const {
        return false;
    }
}

namespace okami::graphics {
    std::unique_ptr<core::ISystem> CreateIm3d(
        core::ISystem* renderer) {
        IRenderer* rendererInterface = nullptr;

        core::InterfaceCollection interfaces(renderer);
        rendererInterface = interfaces.Query<IRenderer>();

        if (!renderer) {
            throw std::runtime_error("Renderer system does not expose "
                "IRenderer interface!");
        }

        return std::make_unique<diligent::Im3dSystem>(rendererInterface);
    }

    std::unique_ptr<core::ISystem> CreateIm3d(
        core::ISystem* renderer,
        core::ISystem* input) {

        diligent::IGLFWWindowProvider* glfw = nullptr;
        IRenderer* rendererInterface = nullptr;

        core::InterfaceCollection interfaces;
        renderer->RegisterInterfaces(interfaces);
        input->RegisterInterfaces(interfaces);

        rendererInterface = interfaces.Query<IRenderer>();

        if (!rendererInterface) {
            throw std::runtime_error("Renderer system does not expose "
                "IRenderer interface!");
        }

        glfw = interfaces.Query<diligent::IGLFWWindowProvider>();

        if (!glfw) {
            throw std::runtime_error("Input system does not expose "
                "IGLFWWindowProvider interface!");
        }

        return std::make_unique<diligent::Im3dSystem>(rendererInterface, glfw);
    }
}