#pragma once

#include <okami/Embed.hpp>

#include <okami/diligent/RenderModule.hpp>
#include <okami/diligent/Buffers.hpp>
#include <okami/diligent/SceneGlobals.hpp>

#include <okami/Resource.hpp>
#include <okami/ResourceManager.hpp>
#include <okami/GraphicsComponents.hpp>

#include <RefCntAutoPtr.hpp>
#include <RenderDevice.h>

namespace DG = Diligent;

namespace okami::graphics::diligent {
    class StaticMeshModule : 
        public IRenderModule,
        public core::IResourceManager<core::StaticMeshMaterial> {
    public:
        struct Pipeline {
            DG::RefCntAutoPtr<DG::IShader>          mPS;
            
            DG::RefCntAutoPtr<DG::IPipelineState>   mState;

            DG::RefCntAutoPtr<DG::IShaderResourceBinding>
                mDefaultBinding;
        };

    private:
        core::VertexFormat                          mFormat;
        DG::RefCntAutoPtr<DG::IShader>              mVS;
        DG::RefCntAutoPtr<DG::ITexture>             mDefaultTexture;

        DynamicUniformBuffer<
            HLSL::StaticInstanceData>               mInstanceData;

        std::unordered_map<RenderPass,
            int, RenderPass::Hasher>                mPipelineLookup;
        std::vector<Pipeline>                       mPipelines;

        core::ResourceManager<
            core::StaticMeshMaterial>               mMaterialManager;

        struct StaticMeshMaterialImpl {
            std::vector<DG::RefCntAutoPtr<
                DG::IShaderResourceBinding>> mBindings;
        };

        StaticMeshMaterialImpl                      mDefaultMaterial;

        void InitializeMaterial(
            const core::StaticMeshMaterial::Data& materialData,
            StaticMeshMaterialImpl& impl);

    public:
        StaticMeshModule();

        void OnDestroy(WeakHandle<core::StaticMeshMaterial> material);
        void OnFinalize(WeakHandle<core::StaticMeshMaterial> material);

        void Startup(
            core::ISystem* renderer,
            DG::IRenderDevice* device,
            const RenderModuleParams& params) override;
        void Update(
            bool bAllowBlock) override;
        void QueueCommands(
            DG::IDeviceContext* context,
            const core::Frame& frame,
            const RenderView& view,
            const RenderPass& pass,
            const RenderModuleGlobals& globals) override;
        void WaitUntilReady(core::SyncObject& obj) override;
        void Shutdown() override;
        void RegisterVertexFormats(core::VertexLayoutRegistry& registry) override;
        void RegisterResourceInterfaces(core::ResourceInterface& resourceInterface) override;
    
        Handle<core::StaticMeshMaterial> Load(
            const std::filesystem::path& path, 
            const core::LoadParams<core::StaticMeshMaterial>& params, 
            resource_id_t newResId) override;
        Handle<core::StaticMeshMaterial> Add(
            core::StaticMeshMaterial&& obj, 
            resource_id_t newResId) override;
        Handle<core::StaticMeshMaterial> Add(
            core::StaticMeshMaterial&& obj, 
            const std::filesystem::path& path, 
            resource_id_t newResId) override;
    };
}