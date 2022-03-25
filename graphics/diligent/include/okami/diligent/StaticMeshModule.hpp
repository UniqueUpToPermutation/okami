#pragma once

#include <okami/Embed.hpp>

#include <okami/diligent/RenderModule.hpp>
#include <okami/diligent/Buffers.hpp>
#include <okami/diligent/ShaderTypes.hpp>

#include <okami/Resource.hpp>
#include <okami/ResourceManager.hpp>
#include <okami/ResourceBackend.hpp>
#include <okami/GraphicsComponents.hpp>

#include <RefCntAutoPtr.hpp>
#include <RenderDevice.h>

namespace DG = Diligent;

namespace okami::graphics::diligent {
    class StaticMeshModule : 
        public IRenderModule {
    public:
        struct Pipeline {
            DG::RefCntAutoPtr<DG::IShader>          mPS;
            
            DG::RefCntAutoPtr<DG::IPipelineState>   mState;

            DG::RefCntAutoPtr<DG::IShaderResourceBinding>
                mDefaultBinding;
        };

        struct StaticMeshMaterialBackend {
            std::vector<DG::RefCntAutoPtr<
                DG::IShaderResourceBinding>>        mBindings;
            HLSL::MaterialDesc                      mDesc;
        };

    private:
        using Material = core::Material<core::StaticMesh>;

        core::VertexFormat                          mFormat;
        DG::RefCntAutoPtr<DG::IShader>              mVS;
        DG::RefCntAutoPtr<DG::ITexture>             mDefaultTexture;

        DynamicUniformBuffer<
            HLSL::StaticInstanceData>               mInstanceData;
        DynamicUniformBuffer<
            HLSL::MaterialDesc>                     mMaterialData;
        DynamicStructuredBuffer<
            HLSL::LightAttribs>                     mLightsData;

        std::unordered_map<RenderPass,
            int, RenderPass::Hasher>                mPipelineLookup;
        std::vector<Pipeline>                       mPipelines;

        core::ResourceBackend<
            Material,
            StaticMeshMaterialBackend>              mMaterialBackend;

        StaticMeshMaterialBackend                   mDefaultMaterial;

        core::ResourceBackend<
            core::Geometry,
            GeometryBackend>*                       mGeometryBackend;
        core::ResourceBackend<
            core::Texture,
            TextureBackend>*                        mTextureBackend;

        void InitializeMaterial(
            const core::MetaSurfaceDesc& materialData,
            StaticMeshMaterialBackend& backend);

    public:
        StaticMeshModule(
            core::ResourceBackend<
                core::Geometry, GeometryBackend>* geometryBackend,
            core::ResourceBackend<
                core::Texture, TextureBackend>* textureBackend);

        void OnFinalize(
            const Material& frontendIn,
            Material& frontendOut,
            StaticMeshMaterialBackend& backend);
        void OnDestroy(
            StaticMeshMaterialBackend& backend);

        void Startup(
            core::ISystem* renderer,
            DG::IRenderDevice* device,
            const RenderModuleParams& params) override;
        void Update(
            core::ResourceManager*) override;
        void QueueCommands(
            DG::IDeviceContext* context,
            const core::Frame& frame,
            const RenderView& view,
            const RenderCanvas& canvas,
            const RenderPass& pass,
            const RenderModuleGlobals& globals) override;
        void WaitUntilReady(core::SyncObject& obj) override;
        void Shutdown() override;
        void RegisterVertexFormats(core::VertexLayoutRegistry& registry) override;
        void RegisterResourceInterfaces(core::ResourceManager& resourceInterface) override;
        bool IsIdle() override;
        void WaitOnPendingTasks() override;
    };
}