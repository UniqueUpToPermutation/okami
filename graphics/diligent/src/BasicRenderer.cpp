#include <okami/BasicRenderer.hpp>
#include <okami/Display.hpp>
#include <okami/Transform.hpp>
#include <okami/GraphicsUtils.hpp>
#include <okami/StaticMesh.hpp>
#include <okami/Embed.hpp>
#include <okami/Shader.hpp>
#include <okami/StaticMesh.hpp>

using namespace Diligent;

// Load shader locations in TEXT into a lookup
void MakeShaderMap(okami::core::file_map_t* map);

namespace okami::graphics {

    void GetEngineInitializationAttribs(
        DG::RENDER_DEVICE_TYPE DeviceType, 
        DG::EngineCreateInfo& EngineCI, 
        DG::SwapChainDesc& SCDesc) {
		SCDesc.ColorBufferFormat            = TEX_FORMAT_RGBA8_UNORM;
        EngineCI.Features.GeometryShaders   = DEVICE_FEATURE_STATE_ENABLED;
        EngineCI.Features.Tessellation 		= DEVICE_FEATURE_STATE_ENABLED;
    
        if (DeviceType == RENDER_DEVICE_TYPE_GL) {
            EngineCI.Features.SeparablePrograms = DEVICE_FEATURE_STATE_ENABLED;
        }
    }

    BasicRenderer::BasicRenderer(core::ISystem* displaySystem, core::ResourceInterface& resources) : 
        mDisplaySystem(displaySystem), 
        mGeometryManager(
            []() { return core::Geometry(); },
            [this](core::Geometry* geo) { OnDestroy(geo); }),
        mTextureManager(
            []() { return core::Texture(); },
            [this](core::Texture* tex) { OnDestroy(tex); }),
        mBaseMaterialManager(
            []() { return core::BaseMaterial(); },
            [this](core::BaseMaterial* mat) { OnDestroy(mat); }) {

        // Associate the renderer with the geometry 
        resources.Register<core::Geometry>(this);
        resources.Register<core::Texture>(this);
        resources.Register<core::BaseMaterial>(this);
    }

    void BasicRenderer::OnDestroy(core::Geometry* geometry) {
        auto impl = reinterpret_cast<GeometryImpl*>(geometry->GetBackend());
        delete impl;
        delete geometry;
    }

    std::unique_ptr<BasicRenderer::GeometryImpl>
        BasicRenderer::MoveToGPU(const core::Geometry& geometry) {
        auto impl = std::make_unique<GeometryImpl>();

        const auto& geoDesc = geometry.GetDesc();
        const auto& data = geometry.DataCPU();

        for (auto& vertBuffer : data.mVertexBuffers) {
            DG::BufferDesc bufDesc;
            bufDesc.BindFlags = BIND_VERTEX_BUFFER;
            bufDesc.CPUAccessFlags = CPU_ACCESS_NONE;
            bufDesc.Usage = USAGE_IMMUTABLE;
            bufDesc.Size = vertBuffer.mDesc.mSizeInBytes;
            bufDesc.Name = "Generated Vertex Buffer";

            BufferData bufData;
            bufData.DataSize = vertBuffer.mDesc.mSizeInBytes;
            bufData.pContext = mContexts[0];
            bufData.pData = vertBuffer.mBytes.data();

            IBuffer* buffer = nullptr;
            mDevice->CreateBuffer(bufDesc, &bufData, &buffer);

            if (!buffer) {
                throw std::runtime_error("Failed to create vertex buffer!");
            }

            impl->mVertexBuffers.emplace_back(buffer);
        }

        if (geoDesc.bIsIndexed) {
            DG::BufferDesc bufDesc;
            bufDesc.BindFlags = BIND_INDEX_BUFFER;
            bufDesc.CPUAccessFlags = CPU_ACCESS_NONE;
            bufDesc.Usage = USAGE_IMMUTABLE;
            bufDesc.Size = data.mIndexBuffer.mDesc.mSizeInBytes;
            bufDesc.Name = "Generated Index Buffer";

            BufferData bufData;
            bufData.DataSize = data.mIndexBuffer.mDesc.mSizeInBytes;
            bufData.pContext = mContexts[0];
            bufData.pData = data.mIndexBuffer.mBytes.data();

            IBuffer* buffer = nullptr;
            mDevice->CreateBuffer(bufDesc, &bufData, &buffer);

            if (!buffer) {
                throw std::runtime_error("Failed to create index buffer!");
            }

            impl->mIndexBuffer.Attach(buffer);
        }

        return impl;
    }

    void BasicRenderer::OnFinalize(core::Geometry* geometry) {
        auto impl = MoveToGPU(*geometry);
        geometry->DeallocCPU();
        geometry->SetBackend(impl.release());
    }

    void BasicRenderer::OnDestroy(core::Texture* tex) {
        auto impl = reinterpret_cast<TextureImpl*>(tex->GetBackend());
        delete impl;
        delete tex;
    }

    std::unique_ptr<BasicRenderer::TextureImpl> 
        BasicRenderer::MoveToGPU(const core::Texture& texture) {
        auto impl = std::make_unique<TextureImpl>();

        const auto& texDesc = texture.GetDesc();
        const auto& data = texture.DataCPU();

        TextureDesc dg_desc;
        dg_desc.BindFlags = BIND_SHADER_RESOURCE;
        dg_desc.CPUAccessFlags = CPU_ACCESS_NONE;
        dg_desc.Width = texDesc.mWidth;
        dg_desc.Height = texDesc.mHeight;
        dg_desc.Depth = texDesc.GetDepth();
        dg_desc.ArraySize = texDesc.GetArraySize();
        dg_desc.Type = ToDiligent(texDesc.mType);
        dg_desc.SampleCount = texDesc.mSampleCount;
        dg_desc.MipLevels = texDesc.mMipLevels;
        dg_desc.Name = "Generated Texture";
        dg_desc.Format = ToDiligent(texDesc.mFormat);

        auto subresources = texDesc.GetSubresourceDescs();

        std::vector<TextureSubResData> subres_data(subresources.size());

        for (uint i = 0; i < subresources.size(); ++i) {
            subres_data[i].DepthStride = subresources[i].mDepthStride;
            subres_data[i].SrcOffset = subresources[i].mSrcOffset;
            subres_data[i].Stride = subresources[i].mStride;
            subres_data[i].pData = &data.mData[subresources[i].mSrcOffset];
        }

        TextureData dg_data;
        dg_data.NumSubresources = subresources.size();
        dg_data.pContext = mContexts[0];
        dg_data.pSubResources = &subres_data[0];

        ITexture* dg_texture = nullptr;
        mDevice->CreateTexture(dg_desc, &dg_data, &dg_texture);
        impl->mTexture.Attach(dg_texture);

        return impl;
    }

    void BasicRenderer::OnFinalize(core::Texture* texture) {
        auto impl = MoveToGPU(*texture);
        texture->DeallocCPU();
        texture->SetBackend(impl.release());
    }

    std::unique_ptr<BasicRenderer::BaseMaterialImpl> 
        BasicRenderer::MoveToGPU(const core::BaseMaterial& material) {
        auto impl = std::make_unique<BasicRenderer::BaseMaterialImpl>();
        auto data = material.GetData();

        DG::ITexture* albedo;
        if (data.mAlbedo) {
            data.mAlbedo->OnLoadEvent().wait();
            albedo = reinterpret_cast<TextureImpl*>(data.mAlbedo->GetBackend())->mTexture;
        } else
            albedo = mDefaultTexture;
       
        DG::IShaderResourceBinding* binding = nullptr;
        mStaticMeshPipeline.mState->CreateShaderResourceBinding(
            &binding, true);

        binding->GetVariableByIndex(DG::SHADER_TYPE_PIXEL, 
            mStaticMeshPipeline.mAlbedoIdx)->Set(
                albedo->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));

        impl->mBinding = binding;
        return impl;
    }

    void BasicRenderer::OnFinalize(core::BaseMaterial* material) {
        auto impl = MoveToGPU(*material);
        material->SetBackend(impl.release());
    }

    void BasicRenderer::OnDestroy(core::BaseMaterial* material) {
        auto impl = reinterpret_cast<BaseMaterialImpl*>(material->GetBackend());
        delete impl;
        delete material;
    }

    void BasicRenderer::Startup(marl::WaitGroup& waitGroup) {
        core::InterfaceCollection displayInterfaces(mDisplaySystem);
        auto windowProvider = displayInterfaces.Query<INativeWindowProvider>();
        auto display = displayInterfaces.Query<IDisplay>();

        if (!windowProvider) {
            throw std::runtime_error("Error: Display does not expose"
                " INativeWindowProvider interface!");
        }

        if (!display) {
            throw std::runtime_error("Error: Display does not expose"
                " IDisplay interface!");
        }

        if (display->GetRequestedBackend() == GraphicsBackend::OPENGL)
            windowProvider->GLMakeContextCurrent();

        // Create the graphics device and swap chain
        DG::IEngineFactory* factory = nullptr;
        DG::IRenderDevice* device  = nullptr;
        std::vector<DG::IDeviceContext*> contexts;
        DG::ISwapChain* swapChain = nullptr;

        CreateDeviceAndSwapChain(display, windowProvider, 
            &factory, &device, contexts, &swapChain, 
            &GetEngineInitializationAttribs);

        mEngineFactory.Attach(factory);
        mDevice.Attach(device);
        mSwapChain.Attach(swapChain);

        for (auto context : contexts) {
            mContexts.emplace_back(DG::RefCntAutoPtr<DG::IDeviceContext>(context));
        }

        mDisplay = display;
        mNativeWindowProvider = windowProvider;
        mBackend = display->GetRequestedBackend();

        mVertexLayouts.Register<core::StaticMesh>(core::VertexLayout::PositionUV());

        // Load shaders
        core::EmbeddedFileLoader fileLoader(&MakeShaderMap);

        ShaderParams paramsStaticMeshVS("BasicVert.vsh", DG::SHADER_TYPE_VERTEX, "Static Mesh VS");
        ShaderParams paramsStaticMeshPS("BasicPixel.psh", DG::SHADER_TYPE_PIXEL, "Static Mesh PS");

        bool bAddLineNumbers = mBackend != GraphicsBackend::OPENGL;

        mStaticMeshPipeline.mVS.Attach(CompileEmbeddedShader(
            mDevice, paramsStaticMeshVS, &fileLoader, bAddLineNumbers));
        mStaticMeshPipeline.mPS.Attach(CompileEmbeddedShader(
            mDevice, paramsStaticMeshPS, &fileLoader, bAddLineNumbers));

        // Create pipeline
        GraphicsPipelineStateCreateInfo PSOCreateInfo;
        PSOCreateInfo.PSODesc.Name = "Static Mesh";
        PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;
        PSOCreateInfo.GraphicsPipeline.NumRenderTargets             = 1;
        PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                = mSwapChain->GetDesc().ColorBufferFormat;
        PSOCreateInfo.GraphicsPipeline.DSVFormat                    = mSwapChain->GetDesc().DepthBufferFormat;
        PSOCreateInfo.GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
        PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = false;

        InputLayoutDiligent layout = ToDiligent(mVertexLayouts.Get<core::StaticMesh>());
        PSOCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = layout.mElements.data();
        PSOCreateInfo.GraphicsPipeline.InputLayout.NumElements = layout.mElements.size();

        PSOCreateInfo.pVS = mStaticMeshPipeline.mVS;
        PSOCreateInfo.pPS = mStaticMeshPipeline.mPS;

        ShaderResourceVariableDesc Vars[] = {
            {SHADER_TYPE_PIXEL, "t_Albedo", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE}
        };
        mStaticMeshPipeline.mAlbedoIdx = 0;

        PSOCreateInfo.PSODesc.ResourceLayout.Variables    = Vars;
        PSOCreateInfo.PSODesc.ResourceLayout.NumVariables = _countof(Vars);

        SamplerDesc SamLinearClampDesc {
            FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, 
            TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP
        };
        ImmutableSamplerDesc ImtblSamplers[] = {
            {SHADER_TYPE_PIXEL, "t_Albedo", SamLinearClampDesc}
        };
        PSOCreateInfo.PSODesc.ResourceLayout.ImmutableSamplers    = ImtblSamplers;
        PSOCreateInfo.PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);
        PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

        DG::IPipelineState* meshPipeline = nullptr;
        mDevice->CreateGraphicsPipelineState(PSOCreateInfo, &meshPipeline);
        mStaticMeshPipeline.mState.Attach(meshPipeline);

        auto basicTexture = core::Texture::Prefabs::SolidColor(16, 16, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        mDefaultTexture = MoveToGPU(basicTexture)->mTexture;
        
        mStaticMeshPipeline.mDefaultBinding = MoveToGPU(core::BaseMaterial())->mBinding;
    }

    void BasicRenderer::RegisterInterfaces(core::InterfaceCollection& interfaces) {
        interfaces.Add<core::IVertexLayoutProvider>(this);
    }

    void BasicRenderer::LoadResources(core::Frame* frame, 
        marl::WaitGroup& waitGroup) {

        mGeometryManager.ScheduleBackend(waitGroup, true);
        mTextureManager.ScheduleBackend(waitGroup, true);
        mBaseMaterialManager.ScheduleBackend(waitGroup, true);
    }

    core::Handle<core::Geometry> BasicRenderer::Load(
        const std::filesystem::path& path, 
        const core::LoadParams<core::Geometry>& params, 
        core::resource_id_t newResId) {

        auto loader = [this](const std::filesystem::path& path, 
            const core::LoadParams<core::Geometry>& params) {
            auto layout = GetVertexLayout(params.mComponentType);
            return core::Geometry::Load(path, layout);
        };

        auto finalizer = [this](core::Geometry* geo) {
            OnFinalize(geo);
        };

        return mGeometryManager.Load(path, params, newResId, loader, finalizer);
    } 

    core::Handle<core::Geometry> BasicRenderer::Add(core::Geometry&& obj, 
        core::resource_id_t newResId) {
        auto finalizer = [this](core::Geometry* geo) {
            OnFinalize(geo);
        };
        return mGeometryManager.Add(std::move(obj), newResId, finalizer);
    }

    core::Handle<core::Geometry> BasicRenderer::Add(core::Geometry&& obj, 
        const std::filesystem::path& path, 
        core::resource_id_t newResId) {
        auto finalizer = [this](core::Geometry* geo) {
            OnFinalize(geo);
        };
        return mGeometryManager.Add(std::move(obj), path, newResId, finalizer);
    }

    core::Handle<core::Texture> BasicRenderer::Load(
        const std::filesystem::path& path, 
        const core::LoadParams<core::Texture>& params, 
        core::resource_id_t newResId) {
        auto loader = [](const std::filesystem::path& path, 
            const core::LoadParams<core::Texture>& params) {
            return core::Texture::Load(path, params);
        };

        auto finalizer = [this](core::Texture* tex) {
            OnFinalize(tex);
        };

        return mTextureManager.Load(path, params, newResId, loader, finalizer);
    }

    core::Handle<core::Texture> BasicRenderer::Add(core::Texture&& obj, 
        core::resource_id_t newResId) {
        auto finalizer = [this](core::Texture* tex) {
            OnFinalize(tex);
        };
        return mTextureManager.Add(std::move(obj), newResId, finalizer);
    }

    core::Handle<core::Texture> BasicRenderer::Add(core::Texture&& obj, 
        const std::filesystem::path& path, 
        core::resource_id_t newResId) {
        auto finalizer = [this](core::Texture* tex) {
            OnFinalize(tex);
        };
        return mTextureManager.Add(std::move(obj), path, newResId, finalizer);
    }

    core::Handle<core::BaseMaterial> BasicRenderer::Load(
        const std::filesystem::path& path, 
        const core::LoadParams<core::BaseMaterial>& params, 
        core::resource_id_t newResId) {
        throw std::runtime_error("Not implemented!");
    }

    core::Handle<core::BaseMaterial> BasicRenderer::Add(core::BaseMaterial&& obj, 
        core::resource_id_t newResId) {
        auto finalizer = [this](core::BaseMaterial* mat) {
            OnFinalize(mat);
        };
        return mBaseMaterialManager.Add(std::move(obj), newResId, finalizer);
    }

    core::Handle<core::BaseMaterial> BasicRenderer::Add(core::BaseMaterial&& obj, 
        const std::filesystem::path& path, 
        core::resource_id_t newResId) {
        auto finalizer = [this](core::BaseMaterial* mat) {
            OnFinalize(mat);
        };
        return mBaseMaterialManager.Add(std::move(obj), path, newResId, finalizer);
    }

    const core::VertexLayout& BasicRenderer::GetVertexLayout(
        const entt::meta_type& type) const {
        return mVertexLayouts.Get(type);
    }

    void BasicRenderer::RequestSync(core::SyncObject& syncObject) {
    }

    void BasicRenderer::BeginExecute(core::Frame* frame, 
        marl::WaitGroup& renderGroup, 
        marl::WaitGroup& updateGroup,
        core::SyncObject& syncObject,
        const core::Time& time) {

        // Schedule the updates of resource managers
        mGeometryManager.ScheduleBackend(updateGroup);
        mTextureManager.ScheduleBackend(updateGroup);
        mBaseMaterialManager.ScheduleBackend(updateGroup);

        auto backBufferRTV = mSwapChain->GetCurrentBackBufferRTV();
        auto depthBufferDSV = mSwapChain->GetDepthBufferDSV();
        auto immediateContext = mContexts[0];

        ITextureView* backBuffers[] = {backBufferRTV};
        float color[] = { 0.5f, 0.5f, 0.5f, 1.0f };

        immediateContext->SetRenderTargets(1, backBuffers, depthBufferDSV, 
            RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        immediateContext->ClearRenderTarget(backBufferRTV, color, 
            RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        auto& registry = frame->Registry();

        auto staticMeshes = registry.view<core::StaticMesh>();

        for (auto entity : staticMeshes) {
            auto& mesh = staticMeshes.get<core::StaticMesh>(entity);

            auto geometryImpl = reinterpret_cast<GeometryImpl*>
                (mesh.mGeometry->GetBackend());

            DG::IBuffer* vertBuffers[] = { geometryImpl->mVertexBuffers[0] };
            DG::Uint64 offsets[] = { 0 };
            immediateContext->SetVertexBuffers(0, 1, vertBuffers, offsets, 
                RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_NONE);
            if (geometryImpl->mIndexBuffer) {
                immediateContext->SetIndexBuffer(geometryImpl->mIndexBuffer, 0, 
                    RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            }

            immediateContext->SetPipelineState(mStaticMeshPipeline.mState);

            if (mesh.mMaterial) {
                auto materialImpl = reinterpret_cast<BaseMaterialImpl*>
                    (mesh.mMaterial->GetBackend());
                immediateContext->CommitShaderResources(
                    materialImpl->mBinding,
                    RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            } else {
                immediateContext->CommitShaderResources(
                    mStaticMeshPipeline.mDefaultBinding,
                    RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            }
            
            const auto& geoDesc = mesh.mGeometry->GetDesc();

            if (geometryImpl->mIndexBuffer) {
                DG::DrawIndexedAttribs attribs;
                attribs.NumIndices = geoDesc.mIndexedAttribs.mNumIndices;
                attribs.IndexType = ToDiligent(geoDesc.mIndexedAttribs.mIndexType);
                immediateContext->DrawIndexed(attribs);
            } else {
                DG::DrawAttribs attribs;
                attribs.NumVertices = geoDesc.mAttribs.mNumVertices;
                immediateContext->Draw(attribs);
            }
        }

        if (mBackend == GraphicsBackend::OPENGL) {
            // For OpenGL with GLFW, we need to call glfwSwapBuffers.
            mNativeWindowProvider->GLSwapBuffers();
        } else {
            mSwapChain->Present();
        }
    }

    void BasicRenderer::EndExecute(core::Frame* frame) {
    }

    void BasicRenderer::Shutdown() {
        mStaticMeshPipeline = StaticMeshPipeline();

        mDefaultTexture.Release();
        mSwapChain.Release();
        mContexts.clear();
        mDevice.Release();
        mEngineFactory.Release();
    }

    std::unique_ptr<core::ISystem> CreateRenderer(core::ISystem* displaySystem, core::ResourceInterface& resources) {
        return std::make_unique<BasicRenderer>(displaySystem, resources);
    }
}