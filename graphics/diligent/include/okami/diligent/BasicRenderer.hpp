#pragma once

#include <okami/System.hpp>
#include <okami/ResourceManager.hpp>
#include <okami/ResourceInterface.hpp>
#include <okami/Geometry.hpp>
#include <okami/Texture.hpp>
#include <okami/Material.hpp>
#include <okami/Graphics.hpp>

#include <okami/diligent/Display.hpp>
#include <okami/diligent/Buffers.hpp>
#include <okami/diligent/SpriteModule.hpp>

#include <RenderDevice.h>
#include <SwapChain.h>
#include <DeviceContext.h>
#include <RefCntAutoPtr.hpp>
#include <BasicMath.hpp>

#include <shaders/BasicTypes.hlsl>

namespace okami::graphics::diligent {
    namespace DG = Diligent;

    class BasicRenderer : 
        public core::ISystem,
        public core::IResourceManager<core::Geometry>,
        public core::IResourceManager<core::Texture>,
        public core::IResourceManager<core::BaseMaterial>,
        public core::IVertexLayoutProvider {
    public:
      struct GeometryImpl {
        std::vector<DG::RefCntAutoPtr<DG::IBuffer>>
            mVertexBuffers;
        DG::RefCntAutoPtr<DG::IBuffer>
            mIndexBuffer;
        };

        struct TextureImpl {
            DG::RefCntAutoPtr<DG::ITexture>
                mTexture;
        };

        struct BaseMaterialImpl {
            DG::RefCntAutoPtr<DG::IShaderResourceBinding>
                mBinding;
        };

    private:
        DG::RefCntAutoPtr<DG::IRenderDevice>        mDevice;
        DG::RefCntAutoPtr<DG::IEngineFactory>       mEngineFactory;
        std::vector<DG::RefCntAutoPtr<DG::IDeviceContext>>   
            mContexts;
        DG::RefCntAutoPtr<DG::ISwapChain>           mSwapChain;
        core::ISystem*                              mDisplaySystem;
        core::VertexLayoutRegistry                  mVertexLayouts;
        INativeWindowProvider*                      mNativeWindowProvider;
        IDisplay*                                   mDisplay;
        GraphicsBackend                             mBackend;

        DG::RefCntAutoPtr<DG::ITexture>             mDefaultTexture;
        struct StaticMeshPipeline {
            DG::RefCntAutoPtr<DG::IShader>          mVS;
            DG::RefCntAutoPtr<DG::IShader>          mPS;
            DG::RefCntAutoPtr<DG::IPipelineState>   mState;

            DG::Uint32                              mAlbedoIdx;
            DG::RefCntAutoPtr<DG::IShaderResourceBinding>
                mDefaultBinding;
        } mStaticMeshPipeline;

        SpriteModule                                mSpriteModule;

        core::ResourceManager<core::Geometry>       mGeometryManager;
        core::ResourceManager<core::Texture>        mTextureManager;
        core::ResourceManager<core::BaseMaterial>   mBaseMaterialManager;

        DynamicUniformBuffer<HLSL::SceneGlobals>          mSceneGlobals;
        DynamicUniformBuffer<HLSL::StaticInstanceData>    mInstanceData;

        std::unique_ptr<GeometryImpl>       MoveToGPU(const core::Geometry& geometry);
        std::unique_ptr<TextureImpl>        MoveToGPU(const core::Texture& texture);
        std::unique_ptr<BaseMaterialImpl>   MoveToGPU(const core::BaseMaterial& material);

        StaticMeshPipeline CreateStaticMeshPipeline(core::IVirtualFileSystem* fileLoader);

    public:
        BasicRenderer(
            core::ISystem* displaySystem, 
            core::ResourceInterface& resources);

        void Render(core::Frame& frame,
            core::SyncObject& syncObject);

        void Startup(marl::WaitGroup& waitGroup) override;
        void Shutdown() override;

        void RegisterInterfaces(
            core::InterfaceCollection& interfaces) override;
        void SetFrame(core::Frame& frame) override;
        void LoadResources(marl::WaitGroup& waitGroup) override;
        void RequestSync(core::SyncObject& syncObject) override;
        void BeginExecute(core::Frame& frame, 
            marl::WaitGroup& renderGroup, 
            marl::WaitGroup& updateGroup,
            core::SyncObject& syncObject,
            const core::Time& time) override;
        void EndExecute(core::Frame& frame) override;

        const core::VertexFormat& GetVertexLayout(
            const entt::meta_type& type) const override;

        // Geometry resource handlers
        void OnFinalize(core::Geometry* geometry);
        void OnDestroy(core::Geometry* geometry);
        core::Handle<core::Geometry> Load(
            const std::filesystem::path& path, 
            const core::LoadParams<core::Geometry>& params, 
            core::resource_id_t newResId) override;
        core::Handle<core::Geometry> Add(core::Geometry&& obj, 
            core::resource_id_t newResId) override;
        core::Handle<core::Geometry> Add(core::Geometry&& obj, 
            const std::filesystem::path& path, 
            core::resource_id_t newResId) override;

        // Texture resource handlers
        void OnFinalize(core::Texture* texture);
        void OnDestroy(core::Texture* texture);
        core::Handle<core::Texture> Load(
            const std::filesystem::path& path, 
            const core::LoadParams<core::Texture>& params, 
            core::resource_id_t newResId) override;
        core::Handle<core::Texture> Add(core::Texture&& obj, 
            core::resource_id_t newResId) override;
        core::Handle<core::Texture> Add(core::Texture&& obj, 
            const std::filesystem::path& path, 
            core::resource_id_t newResId) override;

        // Base material handlers
        void OnFinalize(core::BaseMaterial* material);
        void OnDestroy(core::BaseMaterial* material);
        core::Handle<core::BaseMaterial> Load(
            const std::filesystem::path& path, 
            const core::LoadParams<core::BaseMaterial>& params, 
            core::resource_id_t newResId) override;
        core::Handle<core::BaseMaterial> Add(core::BaseMaterial&& obj, 
            core::resource_id_t newResId) override;
        core::Handle<core::BaseMaterial> Add(core::BaseMaterial&& obj, 
            const std::filesystem::path& path, 
            core::resource_id_t newResId) override;
    };
}