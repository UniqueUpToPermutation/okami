#include <okami/diligent/StaticMeshModule.hpp>
#include <okami/Geometry.hpp>
#include <okami/diligent/GraphicsUtils.hpp>
#include <okami/Frame.hpp>
#include <okami/diligent/Shader.hpp>

using namespace Diligent;
using namespace okami::core;

namespace okami::graphics::diligent {

    StaticMeshModule::Pipeline CreateStaticMeshPipeline(
        DG::IRenderDevice* device,
        IRenderer* renderer,
        IGlobalsBufferProvider* globalsBufferProvider,
        DG::IShader* vertexShader,
        DynamicUniformBuffer<HLSL::StaticInstanceData>* instanceBuffer,
        const VertexFormat& vertexFormat,
        const RenderPass& pass,
        const RenderModuleParams& params) {
        
        auto fileLoader = params.mFileSystem;

        ShaderParams paramsStaticMeshPSColor(
            "BasicPixel.psh", DG::SHADER_TYPE_PIXEL, "Static Mesh PS");

        StaticMeshModule::Pipeline pipeline;
        pipeline.mPS.Attach(CompileEmbeddedShader(
            device, paramsStaticMeshPSColor, fileLoader, true));

        // Create pipeline
        GraphicsPipelineStateCreateInfo PSOCreateInfo;
        PSOCreateInfo.PSODesc.Name = "Static Mesh";
        PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;
        PSOCreateInfo.GraphicsPipeline.NumRenderTargets             = pass.mAttributeCount;
        for (int i = 0; i < pass.mAttributeCount; ++i) {
            PSOCreateInfo.GraphicsPipeline.RTVFormats[0]            = ToDiligent(renderer->GetFormat(pass.mAttributes[i]));
        }
        PSOCreateInfo.GraphicsPipeline.DSVFormat                    = ToDiligent(renderer->GetDepthFormat(pass));
        PSOCreateInfo.GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_BACK;
        PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = true;

        InputLayoutDiligent inputLayout = ToDiligent(vertexFormat);
        PSOCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = inputLayout.mElements.data();
        PSOCreateInfo.GraphicsPipeline.InputLayout.NumElements = inputLayout.mElements.size();

        PSOCreateInfo.pVS = vertexShader;

        std::vector<ShaderResourceVariableDesc> shaderVars;
        std::vector<ImmutableSamplerDesc> samplerDescs;

        SamplerDesc SamLinearClampDesc {
            FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, 
            TEXTURE_ADDRESS_WRAP, TEXTURE_ADDRESS_WRAP, TEXTURE_ADDRESS_WRAP
        };

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
        pipeline.mState->GetStaticVariableByName(
            DG::SHADER_TYPE_VERTEX, "cbuf_SceneGlobals")->Set(
                globalsBufferProvider->GetGlobalsBuffer()->Get());
        
        if (pass.mAttributeCount > 0) {
            pipeline.mState->GetStaticVariableByName(
                DG::SHADER_TYPE_VERTEX, "cbuf_InstanceData")->Set(instanceBuffer->Get());
            pipeline.mState->GetStaticVariableByName(
                DG::SHADER_TYPE_VERTEX, "cbuf_InstanceData")->Set(instanceBuffer->Get());
        }

        return pipeline;
    }

    StaticMeshModule::StaticMeshModule() :
        mMaterialManager(
            []() { return StaticMeshMaterial(); },
            [this](StaticMeshMaterial* mat) { OnDestroy(mat); }),
        mFormat(core::VertexFormat::PositionUVNormal()) {
    }
    
    void StaticMeshModule::Startup(
        core::ISystem* renderer,
        DG::IRenderDevice* device,
        const RenderModuleParams& params) {

        ShaderParams paramsStaticMeshVS(
            "BasicVert.vsh", DG::SHADER_TYPE_VERTEX, "Static Mesh VS");
        mVS.Attach(CompileEmbeddedShader(
            device, paramsStaticMeshVS, params.mFileSystem, true));

        mInstanceData = DynamicUniformBuffer<HLSL::StaticInstanceData>(device, 1);
        mDefaultTexture = params.mDefaultTexture;

        core::InterfaceCollection interfaces(renderer);
        auto rendererInterface = interfaces.Query<IRenderer>();
        auto globalBuffersPovider = interfaces.Query<IGlobalsBufferProvider>();

        if (!rendererInterface) {
            throw std::runtime_error("Renderer does not implement"
                " IRenderer!");
        }

        if (!globalBuffersPovider) {
            throw std::runtime_error("Renderer does not implement"
                " IGlobalsBufferProvider!");
        }

        int pipelineId = 0;
        for (auto pass : params.mRequestedRenderPasses) {
            auto pipeline = CreateStaticMeshPipeline(
                device,
                rendererInterface,
                globalBuffersPovider,
                mVS,
                &mInstanceData,
                mFormat,
                pass,
                params);

            mPipelineLookup.emplace(pass, pipelineId++);
            mPipelines.emplace_back(std::move(pipeline));
        }
    }

    void StaticMeshModule::QueueCommands(
        DG::IDeviceContext* context,
        const core::Frame& frame,
        const RenderView& view,
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

        auto staticMeshes = registry.view<core::StaticMesh>();

        struct RenderCall {
            DG::float4x4 mWorldTransform;
            core::StaticMesh mStaticMesh;
        };

        for (auto entity : staticMeshes) {
            auto& staticMesh = staticMeshes.get<core::StaticMesh>(entity);
            auto transform = registry.try_get<core::Transform>(entity);

            RenderCall call;
            call.mStaticMesh = staticMesh;
            if (transform) {
                call.mWorldTransform = ToMatrix(*transform);
            } else {
                call.mWorldTransform = DG::float4x4::Identity();
            }
            
            auto geometryImpl = reinterpret_cast<GeometryImpl*>
                (call.mStaticMesh.mGeometry->GetBackend());

            // Setup vertex buffers
            DG::IBuffer* vertBuffers[] = { geometryImpl->mVertexBuffers[0] };
            DG::Uint64 offsets[] = { 0 };
            context->SetVertexBuffers(0, 1, vertBuffers, offsets, 
                RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_NONE);
            if (geometryImpl->mIndexBuffer) {
                context->SetIndexBuffer(geometryImpl->mIndexBuffer, 0, 
                    RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            }

            // Bind shader resources
            if (call.mStaticMesh.mMaterial) {
                auto materialImpl = reinterpret_cast<StaticMeshMaterialImpl*>
                    (call.mStaticMesh.mMaterial->GetBackend());
                context->CommitShaderResources(
                    materialImpl->mBindings[pipelineId],
                    RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            } else {
                context->CommitShaderResources(
                    mDefaultMaterial.mBindings[pipelineId],
                    RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            }

            HLSL::StaticInstanceData instanceData;
            instanceData.mWorld = call.mWorldTransform;
            instanceData.mEntity = (int32_t)entity;

            // Submit instance data to the GPU
            mInstanceData.Write(context, instanceData);
            
            // Submit draw call to GPU
            const auto& geoDesc = call.mStaticMesh.mGeometry->GetDesc();
            if (geometryImpl->mIndexBuffer) {
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
        mVS.Release();
        mInstanceData = DynamicUniformBuffer<HLSL::StaticInstanceData>();
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
        core::ResourceInterface& resourceInterface) {
        resourceInterface.Register<StaticMeshMaterial>(this);
    }
    
    Handle<core::StaticMeshMaterial> StaticMeshModule::Load(
        const std::filesystem::path& path, 
        const core::LoadParams<core::StaticMeshMaterial>& params, 
        resource_id_t newResId) {
        throw std::runtime_error("Not supported for StaticMeshMaterial!");
    }

    Handle<core::StaticMeshMaterial> StaticMeshModule::Add(
        core::StaticMeshMaterial&& obj, 
        resource_id_t newResId) {

        auto finalizer = [this](StaticMeshMaterial* mat) {
            OnFinalize(mat);
        };

        mMaterialManager.Add(std::move(obj),
            newResId,
            finalizer);
    }

    Handle<core::StaticMeshMaterial> StaticMeshModule::Add(
        core::StaticMeshMaterial&& obj, 
        const std::filesystem::path& path, 
        resource_id_t newResId) {
        throw std::runtime_error("Not supported for StaticMeshMaterial!");
    }

    void StaticMeshModule::OnDestroy(StaticMeshMaterial* material) {
        auto impl = reinterpret_cast<StaticMeshMaterialImpl*>(material->GetBackend());
        delete impl;
        delete material;
    }

    void StaticMeshModule::OnFinalize(core::StaticMeshMaterial* material) {
        auto& data = material->GetData();

        auto impl = std::make_unique<StaticMeshMaterialImpl>();
        auto data = material->GetData();

        DG::ITexture* albedo;
        if (data.mAlbedo) {
            data.mAlbedo->OnLoadEvent().wait();
            albedo = GetTextureImpl(data.mAlbedo)->mTexture;
        } else
            albedo = mDefaultTexture;
       
        for (auto& pipeline : mPipelines) {
            DG::IShaderResourceBinding* binding = nullptr;
            pipeline.mState->CreateShaderResourceBinding(
                &binding, true);

            binding->GetVariableByIndex(DG::SHADER_TYPE_PIXEL, 
                pipeline.mAlbedoIdx)->Set(
                    albedo->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
        
            DG::RefCntAutoPtr<DG::IShaderResourceBinding> autoPtr(binding);
            impl->mBindings.emplace_back(std::move(autoPtr));
        }
    }
}