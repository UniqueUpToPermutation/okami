#include <okami/GraphicsComponents.hpp>
#include <okami/Transform.hpp>

#include <okami/diligent/GraphicsUtils.hpp>
#include <okami/diligent/SpriteModule.hpp>

namespace okami::graphics::diligent {
    void SpriteModule::Startup(DG::IRenderDevice* device,
        DG::ISwapChain* swapChain, 
        DynamicUniformBuffer<HLSL::SceneGlobals>& globals,
        core::IVirtualFileSystem* vfilesystem) {

        auto shaders = SpriteShaders::LoadDefaults(device, vfilesystem);
        mPipeline = SpriteBatchPipeline(
            device,
            globals,
            shaders,
            swapChain->GetDesc().ColorBufferFormat,
            swapChain->GetDesc().DepthBufferFormat,
            1,
            DG::FILTER_TYPE_LINEAR);

        mState = SpriteBatchState(mPipeline);
        mBatch = SpriteBatch(device);
    }

    void SpriteModule::RequestSync(core::SyncObject& syncObject) {
    }

    void SpriteModule::Shutdown() {
        mBatch = SpriteBatch();
        mState = SpriteBatchState();
        mPipeline = SpriteBatchPipeline();
    }
}