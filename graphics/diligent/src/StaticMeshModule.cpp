#include <okami/diligent/StaticMeshModule.hpp>
#include <okami/Geometry.hpp>
#include <okami/diligent/GraphicsUtils.hpp>
#include <okami/Frame.hpp>
#include <okami/diligent/Shader.hpp>

using namespace Diligent;
using namespace okami::core;

#define VS_SOURCE "StaticMesh.vsh"
#define PS_SOURCE "Default.psh"

#define LIGHT_BUFFER_SIZE 4

namespace okami::graphics::diligent {

    StaticMeshModule::Pipeline CreateStaticMeshPipeline(
        DG::IRenderDevice* device,
        IRenderPassFormatProvider* formatProvider,
        IGlobalsBufferProvider* globalsBufferProvider,
        DG::IShader* vertexShader,
        DynamicUniformBuffer<HLSL::StaticInstanceData>* instanceBuffer,
        DynamicUniformBuffer<HLSL::MaterialDesc>* materialBuffer,
        DynamicStructuredBuffer<HLSL::LightAttribs>* lightsBuffer,
        const VertexFormat& vertexFormat,
        const RenderPass& pass,
        const RenderModuleParams& params) {
        
        auto fileLoader = params.mFileSystem;

        StaticMeshModule::Pipeline pipeline;

        if (pass.mAttributeCount > 0) {
            ShaderPreprocessorConfig preprocessorConfig;
            WritePassShaderMacros(pass, preprocessorConfig);

            ShaderParams paramsStaticMeshPSColor(
                PS_SOURCE, DG::SHADER_TYPE_PIXEL, 
                "Static Mesh PS", preprocessorConfig);

            pipeline.mPS.Attach(CompileEmbeddedShader(
                device, paramsStaticMeshPSColor, fileLoader, true));
        }

        // Create pipeline
        GraphicsPipelineStateCreateInfo PSOCreateInfo;
        PSOCreateInfo.PSODesc.Name = "Static Mesh";
        PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;
        PSOCreateInfo.GraphicsPipeline.NumRenderTargets             = pass.mAttributeCount;
        PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                = DG::TEX_FORMAT_UNKNOWN;
        for (int i = 0; i < pass.mAttributeCount; ++i) {
            PSOCreateInfo.GraphicsPipeline.RTVFormats[0]            = formatProvider->GetFormat(pass.mAttributes[i]);
        }
        PSOCreateInfo.GraphicsPipeline.DSVFormat                    = formatProvider->GetDepthFormat(pass);
        PSOCreateInfo.GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_BACK;
        PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = true;
        PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.StencilEnable = false;

        InputLayoutDiligent inputLayout = ToDiligent(vertexFormat);
        PSOCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = inputLayout.mElements.data();
        PSOCreateInfo.GraphicsPipeline.InputLayout.NumElements = inputLayout.mElements.size();

        PSOCreateInfo.pVS = vertexShader;

        if (pass.mAttributeCount > 0) {
            PSOCreateInfo.pPS = pipeline.mPS;
        }

        std::vector<ShaderResourceVariableDesc> shaderVars;
        std::vector<ImmutableSamplerDesc> samplerDescs;

        SamplerDesc SamLinearClampDesc {
            FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, 
            TEXTURE_ADDRESS_WRAP, TEXTURE_ADDRESS_WRAP, TEXTURE_ADDRESS_WRAP
        };

        // Vertex shaders
        shaderVars.emplace_back(ShaderResourceVariableDesc{
            SHADER_TYPE_VERTEX, "cbuf_SceneGlobals", 
            SHADER_RESOURCE_VARIABLE_TYPE_STATIC});
        shaderVars.emplace_back(ShaderResourceVariableDesc{
            SHADER_TYPE_VERTEX, "cbuf_InstanceData", 
            SHADER_RESOURCE_VARIABLE_TYPE_STATIC});

        if (pass.mAttributeCount > 0) {
            shaderVars.emplace_back(ShaderResourceVariableDesc{
                SHADER_TYPE_PIXEL, "t_Albedo", 
                SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE});

            shaderVars.emplace_back(ShaderResourceVariableDesc{
                SHADER_TYPE_PIXEL, "cbuf_InstanceData", 
                SHADER_RESOURCE_VARIABLE_TYPE_STATIC});
            shaderVars.emplace_back(ShaderResourceVariableDesc{
                SHADER_TYPE_PIXEL, "cbuf_SceneGlobals", 
                SHADER_RESOURCE_VARIABLE_TYPE_STATIC});
            shaderVars.emplace_back(ShaderResourceVariableDesc{
                SHADER_TYPE_PIXEL, "cbuf_MaterialData", 
                SHADER_RESOURCE_VARIABLE_TYPE_STATIC});
            shaderVars.emplace_back(ShaderResourceVariableDesc{
                SHADER_TYPE_PIXEL, "sbuf_Lights", 
                SHADER_RESOURCE_VARIABLE_TYPE_STATIC});

            samplerDescs.emplace_back(ImmutableSamplerDesc{
                SHADER_TYPE_PIXEL, "t_Albedo", SamLinearClampDesc});
        }

        PSOCreateInfo.PSODesc.ResourceLayout.Variables    = &shaderVars[0];
        PSOCreateInfo.PSODesc.ResourceLayout.NumVariables = shaderVars.size();
        PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

        PSOCreateInfo.PSODesc.ResourceLayout.ImmutableSamplers    = &samplerDescs[0];
        PSOCreateInfo.PSODesc.ResourceLayout.NumImmutableSamplers = samplerDescs.size();

        DG::IPipelineState* pipelineState = nullptr;
        device->CreateGraphicsPipelineState(PSOCreateInfo, &pipelineState);
        pipeline.mState.Attach(pipelineState);

        // Set the scene globals buffer
        auto globalsBuffer = globalsBufferProvider->GetGlobalsBuffer()->Get();
        pipeline.mState->GetStaticVariableByName(
            DG::SHADER_TYPE_VERTEX, "cbuf_SceneGlobals")->Set(globalsBuffer);
    
        pipeline.mState->GetStaticVariableByName(
            DG::SHADER_TYPE_VERTEX, "cbuf_InstanceData")->Set(instanceBuffer->Get());

        if (pass.mAttributeCount > 0) {
            pipeline.mState->GetStaticVariableByName(
                DG::SHADER_TYPE_PIXEL, "cbuf_InstanceData")->Set(instanceBuffer->Get());
            pipeline.mState->GetStaticVariableByName(
                DG::SHADER_TYPE_PIXEL, "cbuf_SceneGlobals")->Set(globalsBuffer);
            pipeline.mState->GetStaticVariableByName(
                DG::SHADER_TYPE_PIXEL, "cbuf_MaterialData")->Set(materialBuffer->Get());
            pipeline.mState->GetStaticVariableByName(
                DG::SHADER_TYPE_PIXEL, "sbuf_Lights")->Set(lightsBuffer->GetView());
        }

        return pipeline;
    }

    StaticMeshModule::StaticMeshModule(
        core::ResourceBackend<
            core::Geometry, GeometryBackend>* geometryBackend,
        core::ResourceBackend<
            core::Texture, TextureBackend>* textureBackend) :
        mFormat(core::VertexFormat::PositionUVNormal()),
        mMaterialBackend(
            [](const Material&) { 
                return StaticMeshMaterialBackend(); 
            },
            nullptr,
            [this](const Material& frontendIn,
                Material& frontendOut,
                StaticMeshMaterialBackend& backend) {
                OnFinalize(frontendIn, frontendOut, backend);
            },
            [this](StaticMeshMaterialBackend& backend) { 
                OnDestroy(backend);
            }),
        mGeometryBackend(geometryBackend),
        mTextureBackend(textureBackend) {        
    }

    void StaticMeshModule::OnFinalize(
        const Material& frontendIn,
        Material& frontendOut,
        StaticMeshMaterialBackend& backend) {

        InitializeMaterial(frontendIn.GetSurface(), backend);
        frontendOut = frontendIn;
    }

    void StaticMeshModule::OnDestroy(
        StaticMeshMaterialBackend& backend) {
        backend.mBindings.clear();
    }
    
    void StaticMeshModule::Startup(
        core::ISystem* renderer,
        DG::IRenderDevice* device,
        const RenderModuleParams& params) {

        ShaderParams paramsStaticMeshVS(
            VS_SOURCE, DG::SHADER_TYPE_VERTEX, "Static Mesh VS");
        mVS.Attach(CompileEmbeddedShader(
            device, paramsStaticMeshVS, params.mFileSystem, true));

        mInstanceData = DynamicUniformBuffer<HLSL::StaticInstanceData>(device);
        mMaterialData = DynamicUniformBuffer<HLSL::MaterialDesc>(device);
        mLightsData = DynamicStructuredBuffer<HLSL::LightAttribs>(device, LIGHT_BUFFER_SIZE);

        mDefaultTexture = params.mDefaultTexture;

        core::InterfaceCollection interfaces(renderer);
        auto globalBuffersProvider = 
            interfaces.Query<IGlobalsBufferProvider>();
        auto renderPassFormatProvider = 
            interfaces.Query<IRenderPassFormatProvider>();

        if (!globalBuffersProvider) {
            throw std::runtime_error("Renderer does not implement"
                " IGlobalsBufferProvider!");
        }

        if (!renderPassFormatProvider) {
            throw std::runtime_error("Renderer does not implement"
                " IRenderPassFormatProvider!");
        }

        int pipelineId = 0;
        for (auto pass : params.mRequestedRenderPasses) {
            auto pipeline = CreateStaticMeshPipeline(
                device,
                renderPassFormatProvider,
                globalBuffersProvider,
                mVS,
                &mInstanceData,
                &mMaterialData,
                &mLightsData,
                mFormat,
                pass,
                params);

            mPipelineLookup.emplace(pass, pipelineId++);
            mPipelines.emplace_back(std::move(pipeline));
        }

        InitializeMaterial(core::MetaSurfaceDesc(), mDefaultMaterial);
    }

    void StaticMeshModule::Update(
        core::ResourceManager*) {
        mMaterialBackend.Run();
    }

    bool StaticMeshModule::IsIdle() {
        return mMaterialBackend.IsIdle();
    }

    void StaticMeshModule::WaitOnPendingTasks() {
        mMaterialBackend.LoadCounter().wait();
    }

    void WriteLightAttribs(
        const core::Transform& transform,
        HLSL::LightAttribs& attribs) {
        attribs.mLightDir = 
            ToDiligent(transform.ApplyToTangent(glm::vec3(0.0f, -1.0f, 0.0f)));
        attribs.mPosition =
            ToDiligent(transform.mTranslation);
        attribs.mScale = 
            (transform.mScale.x + transform.mScale.y + transform.mScale.z) / 3.0;
    }

    void WriteLightAttribs(
        const core::PointLight& light,
        HLSL::LightAttribs& attribs) {
        attribs.mLightType = HLSL_LIGHT_TYPE_POINT;
        attribs.mIrradiance = ToDiligent(light.mColor * light.mRadiantFlux / (4.0f * PI_F));
        attribs.mRadianceFalloff = light.mRadianceFalloff;
    }

    void WriteLightAttribs(
        const core::DirectionalLight& light,
        HLSL::LightAttribs& attribs) {
        attribs.mLightType = HLSL_LIGHT_TYPE_DIRECTIONAL;
        attribs.mIrradiance = ToDiligent(light.mColor * light.mIrradiance);
        attribs.mRadianceFalloff = 0.0;
    }

    void StaticMeshModule::QueueCommands(
        DG::IDeviceContext* context,
        const core::Frame& frame,
        const RenderView& view,
        const RenderCanvas& target,
        const RenderPass& pass,
        const RenderModuleGlobals& globals) {

        auto it = mPipelineLookup.find(pass);

        if (it == mPipelineLookup.end()) {
            throw std::runtime_error("Pass was not requested at Startup!");
        }

        int pipelineId = it->second;
        auto& pipeline = mPipelines[pipelineId];
        const auto& registry = frame.Registry();

        context->SetPipelineState(pipeline.mState);

        struct RenderCall {
            DG::float4x4 mWorldTransform;
            core::StaticMesh mStaticMesh;
        };

        // Populate lights buffer
        auto directionalLights = frame.Registry().view<DirectionalLight, Transform>();
        auto pointLights = frame.Registry().view<PointLight, Transform>();

        HLSL::LightAttribs lights[LIGHT_BUFFER_SIZE];
        int lightIndex = 0;
        for (auto e : directionalLights) {
            if (lightIndex >= LIGHT_BUFFER_SIZE)
                break;
            WriteLightAttribs(directionalLights.get<const Transform>(e), lights[lightIndex]);
            WriteLightAttribs(directionalLights.get<const DirectionalLight>(e), lights[lightIndex]);
            ++lightIndex;
        }

        for (auto e : pointLights) {
            if (lightIndex >= LIGHT_BUFFER_SIZE)
                break;
            WriteLightAttribs(pointLights.get<const Transform>(e), lights[lightIndex]);
            WriteLightAttribs(pointLights.get<const PointLight>(e), lights[lightIndex]);
            ++lightIndex;
        }

        for (; lightIndex < LIGHT_BUFFER_SIZE; lightIndex++) {
            lights[lightIndex].mLightType = HLSL_LIGHT_TYPE_NONE;
        }
        mLightsData.Write(context, lights, LIGHT_BUFFER_SIZE);

        auto staticMeshes = registry.view<core::StaticMesh>();
        for (auto entity : staticMeshes) {
            const auto& staticMesh = staticMeshes.get<const core::StaticMesh>(entity);
            auto transform = registry.try_get<core::Transform>(entity);

            RenderCall call;
            call.mStaticMesh = staticMesh;
            if (transform) {
                call.mWorldTransform = ToMatrix(*transform);
            } else {
                call.mWorldTransform = DG::float4x4::Identity();
            }

            auto geo = mGeometryBackend->TryGet(staticMesh.mGeometry);
            if (!geo)
                continue;

            // Setup vertex buffers
            DG::IBuffer* vertBuffers[] = { 
                geo->mVertexBuffers[0] 
            };
            DG::Uint64 offsets[] = { 0 };
            context->SetVertexBuffers(0, 1, vertBuffers, offsets, 
                RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_NONE);
            if (geo->mIndexBuffer) {
                context->SetIndexBuffer(geo->mIndexBuffer, 0, 
                    RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            }

            // Bind shader resources for material
            auto mat = mMaterialBackend.TryGet(call.mStaticMesh.mMaterial);
            if (mat) {
                context->CommitShaderResources(
                    mat->mBindings[pipelineId],
                    RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
                mMaterialData.Write(context, mat->mDesc);
            } else {
                context->CommitShaderResources(
                    mDefaultMaterial.mBindings[pipelineId],
                    RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
                mMaterialData.Write(context, mDefaultMaterial.mDesc);
            }
            
            HLSL::StaticInstanceData instanceData;
            instanceData.mWorld = call.mWorldTransform;
            instanceData.mEntity = (int32_t)entity;

            // Submit instance data to the GPU
            mInstanceData.Write(context, instanceData);
            
            // Submit draw call to GPU
            const auto& geoDesc = geo->mDesc;
            if (geo->mIndexBuffer) {
                DG::DrawIndexedAttribs attribs;
                attribs.NumIndices = geoDesc.mIndexedAttribs.mNumIndices;
                attribs.IndexType = ToDiligent(geoDesc.mIndexedAttribs.mIndexType);

                context->DrawIndexed(attribs);
            } else {
                DG::DrawAttribs attribs;
                attribs.NumVertices = geoDesc.mAttribs.mNumVertices;
                context->Draw(attribs);
            }

            DG::ITexture* texture;
        }
    }

    void StaticMeshModule::Shutdown() {
        mDefaultMaterial = StaticMeshMaterialBackend();
        mVS.Release();
        mInstanceData = DynamicUniformBuffer<HLSL::StaticInstanceData>();
        mMaterialData = DynamicUniformBuffer<HLSL::MaterialDesc>();
        mLightsData = DynamicStructuredBuffer<HLSL::LightAttribs>();
        mPipelines.clear();
    }

    void StaticMeshModule::WaitUntilReady(core::SyncObject& obj) {
        obj.WaitUntilFinished<core::StaticMesh>();
        obj.WaitUntilFinished<core::Transform>();
    }

    void StaticMeshModule::RegisterVertexFormats(
        core::VertexLayoutRegistry& registry) {
        registry.Register<core::StaticMesh>(mFormat);
    }

    void StaticMeshModule::RegisterResourceInterfaces(
        core::ResourceManager& resourceInterface) {
        resourceInterface.Register<Material>(
            &mMaterialBackend);
    }

    int ConvertToHLSL(const core::SurfaceType type) {
        switch (type) {
            case core::SurfaceType::FLAT:
                return MATERIAL_TYPE_FLAT;
            case core::SurfaceType::LAMBERT:
                return MATERIAL_TYPE_LAMBERT;
            case core::SurfaceType::PHONG:
                return MATERIAL_TYPE_PHONG;
            case core::SurfaceType::COOK_TORRENCE:
                return MATERIAL_TYPE_COOK_TORRENCE;
            default:
                throw std::runtime_error("Unrecognized enum!");
        }
    }

    HLSL::MaterialDesc ConvertToHLSL(const core::MetaSurfaceDesc& desc) {
        HLSL::MaterialDesc result;
        result.mAlbedoFactor = ToDiligent(desc.mAlbedoFactor);
        result.mMaterialType = ConvertToHLSL(desc.mType);
        result.mMetallicFactor = desc.mMetallicFactor;
        result.mRoughnessFactor = desc.mRoughnessFactor;
        result.mSpecularPower = desc.mSpecularPower;
        return result;
    }

    void StaticMeshModule::InitializeMaterial(
        const core::MetaSurfaceDesc& data,
        StaticMeshMaterialBackend& impl) {

        auto getTexture = [
            textures = mTextureBackend, 
            &defaultTex = mDefaultTexture](resource_id_t id) {
            auto backend = textures->TryGet(id);
            if (backend) {
                backend->mEvent->wait();
                return backend->mTexture;
            } else {
                return defaultTex;
            }
        };

        auto albedo = getTexture(data.mAlbedo);
       
        for (auto& pipeline : mPipelines) {
            DG::IShaderResourceBinding* binding = nullptr;
            pipeline.mState->CreateShaderResourceBinding(
                &binding, true);

            if (pipeline.mPS) {
                binding->GetVariableByName(DG::SHADER_TYPE_PIXEL, 
                    "t_Albedo")->Set(albedo->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
            }

            DG::RefCntAutoPtr<DG::IShaderResourceBinding> autoPtr(binding);
            impl.mBindings.emplace_back(std::move(autoPtr));
        }

        impl.mDesc = ConvertToHLSL(data);
    }
}