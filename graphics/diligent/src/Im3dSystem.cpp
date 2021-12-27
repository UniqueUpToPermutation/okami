#include <okami/diligent/Im3dSystem.hpp>

#include <iostream>

// Load shader locations in TEXT into a lookup
void MakeShaderMap(okami::core::file_map_t* map);

namespace okami::graphics::diligent {
    void Im3dRenderOverlay::Startup(core::ISystem* renderer, 
        DG::IRenderDevice* device, 
        DG::ISwapChain* swapChain) {

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
            mShaders);

        mModule = Im3dModule(device);
    }

    void Im3dRenderOverlay::QueueCommands(DG::IDeviceContext* context) {
        if (bDraw) {
            mModule.Draw(context, mPipeline);
            bDraw = false;
        }
    }

    void Im3dRenderOverlay::Shutdown() {
        mModule = Im3dModule();
        mPipeline = Im3dPipeline();
        mShaders = Im3dShaders();
    }

    Im3dSystem::Im3dSystem(IRenderer* renderer) :
        mRenderer(renderer) {   
        mRenderer->AddOverlay(&mOverlay);   
    }
    Im3dSystem::~Im3dSystem() {
        mRenderer->RemoveOverlay(&mOverlay);
    }

    core::delegate_handle_t Im3dSystem::Add(immedate_callback_t callback) {
        return mOnUpdate.Add(std::move(callback));
    }
    void Im3dSystem::Remove(core::delegate_handle_t handle) {
        mOnUpdate.Remove(handle);
    }
    marl::WaitGroup& Im3dSystem::GetUpdateWaitGroup() {
        return mUpdateWaitGroup;
    }

    void Im3dSystem::Startup(marl::WaitGroup& waitGroup) {
    }
    void Im3dSystem::RegisterInterfaces(core::InterfaceCollection& interfaces) {
        interfaces.Add<IIm3dCallback>(this);
    }
    void Im3dSystem::Shutdown() {
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
        mUpdateFinished.clear();
        marl::schedule([
            renderer = mRenderer,
            &update = mOnUpdate,
            &overlay = mOverlay,
            updateWait = mUpdateWaitGroup,
            updateFinished = mUpdateFinished]() {
            defer(updateFinished.signal());

            renderer->Wait();
            updateWait.wait();

            Im3d::NewFrame();
            update();
            Im3d::EndFrame();

            overlay.bDraw = true;
        });
    }
    void Im3dSystem::Join(core::Frame& frame) {
        Wait();
    }
    void Im3dSystem::Wait() {
        mUpdateFinished.wait();
    }
}

namespace okami::graphics {
    std::unique_ptr<core::ISystem> CreateIm3d(
        IRenderer* renderer) {
        return std::make_unique<diligent::Im3dSystem>(renderer);
    }
}