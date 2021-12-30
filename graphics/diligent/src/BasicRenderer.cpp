
#include <okami/Transform.hpp>
#include <okami/GraphicsComponents.hpp>
#include <okami/Embed.hpp>

#include <okami/diligent/BasicRenderer.hpp>
#include <okami/diligent/Display.hpp>
#include <okami/diligent/Shader.hpp>
#include <okami/diligent/GraphicsUtils.hpp>

using namespace Diligent;

// Load shader locations in TEXT into a lookup
void MakeShaderMap(okami::core::file_map_t* map);

namespace okami::graphics::diligent {

    inline DG::ITexture* GetDiligentTextureImpl(void* backend) {
        return reinterpret_cast<BasicRenderer::TextureImpl*>(backend)->mTexture;
    }

    void GetEngineInitializationAttribs(
        DG::RENDER_DEVICE_TYPE DeviceType, 
        DG::EngineCreateInfo& EngineCI, 
        DG::SwapChainDesc& SCDesc) {
        
		SCDesc.ColorBufferFormat            = TEX_FORMAT_RGBA8_UNORM_SRGB;
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

        mNativeWindowProvider = windowProvider;
        mDisplay = display;

        mBackend = mDisplay->GetRequestedBackend();
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
        auto impl = reinterpret_cast<BasicRenderer::TextureImpl*>(tex->GetBackend());
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
            albedo = GetDiligentTextureImpl(data.mAlbedo->GetBackend());
        } else
            albedo = mDefaultTexture;
       
        DG::IShaderResourceBinding* binding = nullptr;
        mStaticMeshPipeline.mStateColor->CreateShaderResourceBinding(
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

    BasicRenderer::StaticMeshPipeline 
        BasicRenderer::CreateStaticMeshPipeline(
            core::IVirtualFileSystem* fileLoader) {

        ShaderParams paramsStaticMeshVS(
            "BasicVert.vsh", DG::SHADER_TYPE_VERTEX, "Static Mesh VS");
        ShaderParams paramsStaticMeshPSColor(
            "BasicPixel.psh", DG::SHADER_TYPE_PIXEL, "Static Mesh PS Color");

        bool bAddLineNumbers = mBackend != GraphicsBackend::OPENGL;

        BasicRenderer::StaticMeshPipeline pipeline;

        pipeline.mVS.Attach(CompileEmbeddedShader(
            mDevice, paramsStaticMeshVS, fileLoader, bAddLineNumbers));
        pipeline.mPSColor.Attach(CompileEmbeddedShader(
            mDevice, paramsStaticMeshPSColor, fileLoader, bAddLineNumbers));

        // Create pipeline
        GraphicsPipelineStateCreateInfo PSOCreateInfo;
        PSOCreateInfo.PSODesc.Name = "Static Mesh";
        PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;
        PSOCreateInfo.GraphicsPipeline.NumRenderTargets             = 1;
        PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                = mSwapChain->GetDesc().ColorBufferFormat;
        PSOCreateInfo.GraphicsPipeline.DSVFormat                    = mSwapChain->GetDesc().DepthBufferFormat;
        PSOCreateInfo.GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_BACK;
        PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = true;

        InputLayoutDiligent layout = ToDiligent(mVertexLayouts.Get<core::StaticMesh>());
        PSOCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = layout.mElements.data();
        PSOCreateInfo.GraphicsPipeline.InputLayout.NumElements = layout.mElements.size();

        PSOCreateInfo.pVS = pipeline.mVS;

        ShaderResourceVariableDesc DepthVars[] = {
            {SHADER_TYPE_VERTEX, "cbuf_SceneGlobals", 
                SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
            {SHADER_TYPE_VERTEX, "cbuf_InstanceData", 
                SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        };

        PSOCreateInfo.PSODesc.ResourceLayout.Variables    = DepthVars;
        PSOCreateInfo.PSODesc.ResourceLayout.NumVariables = _countof(DepthVars);
        PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

        DG::IPipelineState* depthPipeline = nullptr;
        mDevice->CreateGraphicsPipelineState(PSOCreateInfo, &depthPipeline);
        pipeline.mStateDepth.Attach(depthPipeline);

        // Create color
        ShaderResourceVariableDesc ColorVars[] = {
            {SHADER_TYPE_PIXEL, "t_Albedo", 
                SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
            {SHADER_TYPE_VERTEX, "cbuf_SceneGlobals", 
                SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
            {SHADER_TYPE_VERTEX, "cbuf_InstanceData", 
                SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
            {SHADER_TYPE_PIXEL, "cbuf_InstanceData", 
                SHADER_RESOURCE_VARIABLE_TYPE_STATIC}
        };
        pipeline.mAlbedoIdx = 0;

        PSOCreateInfo.PSODesc.ResourceLayout.Variables    = ColorVars;
        PSOCreateInfo.PSODesc.ResourceLayout.NumVariables = _countof(ColorVars);

        SamplerDesc SamLinearClampDesc {
            FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, 
            TEXTURE_ADDRESS_WRAP, TEXTURE_ADDRESS_WRAP, TEXTURE_ADDRESS_WRAP
        };
        ImmutableSamplerDesc ImtblSamplers[] = {
            {SHADER_TYPE_PIXEL, "t_Albedo", SamLinearClampDesc}
        };
        PSOCreateInfo.PSODesc.ResourceLayout.ImmutableSamplers    = ImtblSamplers;
        PSOCreateInfo.PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);

        PSOCreateInfo.pPS = pipeline.mPSColor;

        DG::IPipelineState* colorPipeline = nullptr;
        mDevice->CreateGraphicsPipelineState(PSOCreateInfo, &colorPipeline);
        pipeline.mStateColor.Attach(colorPipeline);

        if (bEntityPickEnabled) {
            PSOCreateInfo.GraphicsPipeline.NumRenderTargets = 2;
            PSOCreateInfo.GraphicsPipeline.RTVFormats[0] = mSwapChain->GetDesc().ColorBufferFormat;
            PSOCreateInfo.GraphicsPipeline.RTVFormats[1] = DG::TEX_FORMAT_R32_SINT;
            PSOCreateInfo.GraphicsPipeline.BlendDesc.RenderTargets[1].BlendEnable = false;
            PSOCreateInfo.GraphicsPipeline.BlendDesc.RenderTargets[1].LogicOperationEnable = false;

            DG::IPipelineState* colorPipelinePick = nullptr;
            mDevice->CreateGraphicsPipelineState(PSOCreateInfo, &colorPipelinePick);
            pipeline.mStateColorEntityPick.Attach(colorPipelinePick);
        }

        // Set the scene globals buffer
        pipeline.mStateDepth->GetStaticVariableByName(
            DG::SHADER_TYPE_VERTEX, "cbuf_SceneGlobals")->Set(mSceneGlobals.Get());
        pipeline.mStateDepth->GetStaticVariableByName(
            DG::SHADER_TYPE_VERTEX, "cbuf_InstanceData")->Set(mInstanceData.Get());
    
        pipeline.mStateColor->GetStaticVariableByName(
            DG::SHADER_TYPE_VERTEX, "cbuf_SceneGlobals")->Set(mSceneGlobals.Get());
        pipeline.mStateColor->GetStaticVariableByName(
            DG::SHADER_TYPE_VERTEX, "cbuf_InstanceData")->Set(mInstanceData.Get());
        pipeline.mStateColor->GetStaticVariableByName(
            DG::SHADER_TYPE_PIXEL, "cbuf_InstanceData")->Set(mInstanceData.Get());
    
        if (bEntityPickEnabled) {
            pipeline.mStateColorEntityPick->GetStaticVariableByName(
                DG::SHADER_TYPE_VERTEX, "cbuf_SceneGlobals")->Set(mSceneGlobals.Get());
            pipeline.mStateColorEntityPick->GetStaticVariableByName(
                DG::SHADER_TYPE_VERTEX, "cbuf_InstanceData")->Set(mInstanceData.Get());
            pipeline.mStateColorEntityPick->GetStaticVariableByName(
                DG::SHADER_TYPE_PIXEL, "cbuf_InstanceData")->Set(mInstanceData.Get());
        }

        return pipeline;
    }

    void BasicRenderer::Request(const entt::meta_type& interfaceType) {
        if (interfaceType == entt::resolve<IEntityPick>()) {

            if (mBackend == GraphicsBackend::OPENGL) {
                throw std::runtime_error("Entity picking not supported in OpenGL!");
            }

            bEntityPickEnabled = true;
        } else {
            throw std::runtime_error("Requested interface not supported!");
        }
    }

    void BasicRenderer::Startup(marl::WaitGroup& waitGroup) {
        // Load shaders
        core::EmbeddedFileLoader fileLoader(&MakeShaderMap);

        if (mDisplay->GetRequestedBackend() == GraphicsBackend::OPENGL)
            mNativeWindowProvider->GLMakeContextCurrent();

        // Create the graphics device and swap chain
        DG::IEngineFactory* factory = nullptr;
        DG::IRenderDevice* device  = nullptr;
        std::vector<DG::IDeviceContext*> contexts;
        DG::ISwapChain* swapChain = nullptr;

        CreateDeviceAndSwapChain(mDisplay, mNativeWindowProvider, 
            &factory, &device, contexts, &swapChain, 
            &GetEngineInitializationAttribs);

        mEngineFactory.Attach(factory);
        mDevice.Attach(device);
        mSwapChain.Attach(swapChain);

        for (auto context : contexts) {
            mContexts.emplace_back(
                DG::RefCntAutoPtr<DG::IDeviceContext>(context));
        }

        mVertexLayouts.Register<core::StaticMesh>(core::VertexFormat::PositionUV());

        // Create globals buffer
        mSceneGlobals = DynamicUniformBuffer<HLSL::SceneGlobals>(mDevice);
        mInstanceData = DynamicUniformBuffer<HLSL::StaticInstanceData>(mDevice);

        mStaticMeshPipeline = CreateStaticMeshPipeline(&fileLoader);

        auto basicTexture = core::Texture::Prefabs::SolidColor(
            16, 16, 
            glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        mDefaultTexture = MoveToGPU(basicTexture)->mTexture;
        mStaticMeshPipeline.mDefaultBindingColor = 
            MoveToGPU(core::BaseMaterial())->mBinding;

        mSpriteModule.Startup(mDevice, mSwapChain, mSceneGlobals, &fileLoader);

        RenderModuleParams params;

        for (auto overlay : mOverlays) {
            overlay->Startup(this, mDevice, mSwapChain, params);
        }

        if (bEntityPickEnabled) {
            DG::TextureDesc desc;
            desc.Type = DG::RESOURCE_DIM_TEX_2D;
            desc.Format = DG::TEX_FORMAT_R32_SINT;
            mEntityPicker = TextureCapturePick(mDevice, desc, 32);

            mEntityResultBuffer.resize(mEntityPicker.GetMaxQueries());
            mEntityQueryBuffer.resize(mEntityPicker.GetMaxQueries());
            mEntityResultPromises.resize(mEntityPicker.GetMaxQueries());
        }
    }

    void BasicRenderer::RegisterInterfaces(core::InterfaceCollection& interfaces) {
        interfaces.Add<core::IVertexLayoutProvider>(this);
        interfaces.Add<IRenderer>(this);
        interfaces.Add<IGlobalsBufferProvider>(this);

        if (bEntityPickEnabled)
            interfaces.Add<IEntityPick>(this);
    }

    void BasicRenderer::LoadResources(marl::WaitGroup& waitGroup) {
        mGeometryManager.ScheduleBackend(waitGroup, true);
        mTextureManager.ScheduleBackend(waitGroup, true);
        mBaseMaterialManager.ScheduleBackend(waitGroup, true);
    }

    void BasicRenderer::SetFrame(core::Frame& frame) {
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

    const core::VertexFormat& BasicRenderer::GetVertexLayout(
        const entt::meta_type& type) const {
        return mVertexLayouts.Get(type);
    }

    void BasicRenderer::RequestSync(core::SyncObject& syncObject) {
        mSpriteModule.RequestSync(syncObject);
    }

    void BasicRenderer::UpdateFramebuffers() {
        glm::uvec2 requestedSize;

        auto sz = mDisplay->GetFramebufferSize();
        auto& scDesc = mSwapChain->GetDesc();
        
        if (sz.x != scDesc.Width || sz.y != scDesc.Height) {
            mSwapChain->Resize(sz.x, sz.y);
        }
        requestedSize = sz;

        if (bEntityPickEnabled) {
            if (!mEntityPickTexture || 
                mEntityPickTexture->GetDesc().Width != requestedSize.x ||
                mEntityPickTexture->GetDesc().Height != requestedSize.y) {

                DG::TextureDesc desc;
                desc.Width = requestedSize.x;
                desc.Height = requestedSize.y;
                desc.MipLevels = 1;
                desc.Type = DG::RESOURCE_DIM_TEX_2D;
                desc.SampleCount = 1;
                desc.Name = "Entity Pick Buffer";
                desc.Usage = DG::USAGE_DEFAULT;
                desc.Format = DG::TEX_FORMAT_R32_SINT;
                desc.BindFlags = DG::BIND_RENDER_TARGET;

                DG::ITexture* entityPickBuffer = nullptr;
                mDevice->CreateTexture(desc, nullptr, &entityPickBuffer);
                mEntityPickTexture.Attach(entityPickBuffer);
            }
        }

        if (!mColorBuffer ||
            mColorBuffer->GetDesc().Width != requestedSize.x ||
            mColorBuffer->GetDesc().Height != requestedSize.y) {
                DG::TextureDesc desc;
                desc.Width = requestedSize.x;
                desc.Height = requestedSize.y;
                desc.MipLevels = 1;
                desc.Type = DG::RESOURCE_DIM_TEX_2D;
                desc.SampleCount = 1;
                desc.Name = "Color Buffer";
                desc.Usage = DG::USAGE_DEFAULT;
                desc.Format = mSwapChain->GetDesc().ColorBufferFormat;
                desc.BindFlags = DG::BIND_RENDER_TARGET;

                DG::ITexture* entityPickBuffer = nullptr;
                mDevice->CreateTexture(desc, nullptr, &entityPickBuffer);
                mColorBuffer.Attach(entityPickBuffer);
        }

        if (!mDepthBuffer ||
            mDepthBuffer->GetDesc().Width != requestedSize.x ||
            mDepthBuffer->GetDesc().Height != requestedSize.y) {
                DG::TextureDesc desc;
                desc.Width = requestedSize.x;
                desc.Height = requestedSize.y;
                desc.MipLevels = 1;
                desc.Type = DG::RESOURCE_DIM_TEX_2D;
                desc.SampleCount = 1;
                desc.Name = "Depth Buffer";
                desc.Usage = DG::USAGE_DEFAULT;
                desc.Format = mSwapChain->GetDesc().DepthBufferFormat;
                desc.BindFlags = DG::BIND_DEPTH_STENCIL;

                DG::ITexture* entityPickBuffer = nullptr;
                mDevice->CreateTexture(desc, nullptr, &entityPickBuffer);
                mDepthBuffer.Attach(entityPickBuffer);
        }
    }

    void BasicRenderer::Render(core::Frame& frame,
        core::SyncObject& syncObject) {

        UpdateFramebuffers();

        syncObject.WaitUntilFinished<core::Transform>();
        syncObject.WaitUntilFinished<core::Camera>();
        syncObject.WaitUntilFinished<core::StaticMesh>();

        struct RenderCall {
            DG::float4x4 mWorldTransform;
            core::StaticMesh mStaticMesh;
        };

        auto& registry = frame.Registry();
        const auto& scDesc = mSwapChain->GetDesc();

        // Queue up render calls and compute transforms
        HLSL::SceneGlobals globals;
        globals.mCamera.mView = DG::float4x4::Identity();
        globals.mCamera.mViewProj = DG::float4x4::Identity();
        globals.mCamera.mViewport = DG::float2(scDesc.Width, scDesc.Height);

        auto cameras = registry.view<core::Camera>();

        if (!cameras.empty()) {
            auto cameraEntity = cameras.front();
            auto camera = cameras.get<core::Camera>(cameraEntity);
            DG::float4x4 projTransform = GetProjection(camera, mSwapChain, 
                mBackend == GraphicsBackend::OPENGL);
            
            auto transform = registry.try_get<core::Transform>(cameraEntity);
            DG::float4x4 viewTransform = DG::float4x4::Identity();
            if (transform) {
                viewTransform = ToMatrix(*transform).Inverse();
            }

            globals.mCamera.mView = viewTransform;
            globals.mCamera.mViewProj = viewTransform * projTransform;
        }

        globals.mCamera.mInvView = globals.mCamera.mView.Inverse();
        globals.mCamera.mInvViewProj = globals.mCamera.mViewProj.Inverse();

        auto staticMeshes = registry.view<core::StaticMesh>();

        auto immediateContext = mContexts[0];

        auto colorBufferRTV = mColorBuffer->GetDefaultView(
            DG::TEXTURE_VIEW_RENDER_TARGET);
        auto depthBufferDSV = mDepthBuffer->GetDefaultView(
            DG::TEXTURE_VIEW_DEPTH_STENCIL);

        // Move globals to GPU
        mSceneGlobals.Write(immediateContext, globals);

        // Set render targets
        ITextureView* entityPickView = nullptr;
        std::vector<ITextureView*> rtvs;
        rtvs.emplace_back(colorBufferRTV);
        if (bEntityPickEnabled) {
            entityPickView = mEntityPickTexture->GetDefaultView(
                DG::TEXTURE_VIEW_RENDER_TARGET);
            rtvs.emplace_back(entityPickView);
        }
        float color[] = { 0.3f, 0.3f, 0.3f, 1.0f };
        immediateContext->SetRenderTargets(rtvs.size(), rtvs.data(), depthBufferDSV, 
            RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        // Clear screen
        immediateContext->ClearRenderTarget(colorBufferRTV, color, 
            RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        immediateContext->ClearDepthStencil(depthBufferDSV,
            DG::CLEAR_DEPTH_FLAG, 1.0f, 0, 
            DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        if (bEntityPickEnabled) {
            float color[] = { 0.0f, 0.0f, 0.0f, 0.0f };
            immediateContext->ClearRenderTarget(entityPickView, color, 
                RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        }

        // Bind pipeline state
        auto staticMeshState = mStaticMeshPipeline.mStateColor;
        if (bEntityPickEnabled)
            staticMeshState = mStaticMeshPipeline.mStateColorEntityPick;
        immediateContext->SetPipelineState(staticMeshState);

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
            immediateContext->SetVertexBuffers(0, 1, vertBuffers, offsets, 
                RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_NONE);
            if (geometryImpl->mIndexBuffer) {
                immediateContext->SetIndexBuffer(geometryImpl->mIndexBuffer, 0, 
                    RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            }

            // Bind shader resources
            if (call.mStaticMesh.mMaterial) {
                auto materialImpl = reinterpret_cast<BaseMaterialImpl*>
                    (call.mStaticMesh.mMaterial->GetBackend());
                immediateContext->CommitShaderResources(
                    materialImpl->mBinding,
                    RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            } else {
                immediateContext->CommitShaderResources(
                    mStaticMeshPipeline.mDefaultBindingColor,
                    RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            }

            HLSL::StaticInstanceData instanceData;
            instanceData.mWorld = call.mWorldTransform;
            instanceData.mEntity = (int32_t)entity;

            // Submit instance data to the GPU
            mInstanceData.Write(immediateContext, instanceData);
            
            // Submit draw call to GPU
            const auto& geoDesc = call.mStaticMesh.mGeometry->GetDesc();
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

            DG::ITexture* texture;
        }

        // Draw sprites
        mSpriteModule.Render<&GetDiligentTextureImpl>(
            immediateContext, frame, syncObject);

        // Remove entity buffer for the overlays
        rtvs = { colorBufferRTV };
        immediateContext->SetRenderTargets(rtvs.size(), rtvs.data(), 
            depthBufferDSV, 
            RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        // Draw overlays (i.e., ImGui, etc.)
        for (auto overlay : mOverlays) {
            overlay->QueueCommands(immediateContext, RenderPass::OVERLAY);
        }

        // Compute any entity pick requests
        if (bEntityPickEnabled) {
            mEntityPickRequests.ConsumerCollect();
            auto sz = mDisplay->GetFramebufferSize();

            // Copy queries from the thread-safe pick request queue
            size_t queryCount = 0;
            while (!mEntityPickRequests.ConsumerIsEmpty()) {
                auto& front = mEntityPickRequests.ConsumerFront();

                if (queryCount < mEntityPicker.GetMaxQueries()) {
                    mEntityQueryBuffer[queryCount] = 
                        glm::vec3(front.mPosition.x, 
                            front.mPosition.y, 
                            0.0f);

                    mEntityResultPromises[queryCount] = 
                        std::move(front.mResult);

                    ++queryCount;
                } else {
                    front.mResult.Set(entt::null);
                }

                mEntityPickRequests.ConsumerPop();
            }

            // Submit pick requests to the GPU 
            if (queryCount > 0) {
                mEntityPicker.SubmitCommands(
                    immediateContext, 
                    mEntityPickTexture, 
                    mEntityQueryBuffer.data(),
                    queryCount);
            }
        }

        // Copy/resolve the color and depth buffers
        auto backBufferRTV = mSwapChain->GetCurrentBackBufferRTV();
        immediateContext->SetRenderTargets(0, nullptr, nullptr, 
            RESOURCE_STATE_TRANSITION_MODE_NONE);

        CopyTextureAttribs copyAttribs;
        copyAttribs.DstTextureTransitionMode = 
            RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
        copyAttribs.SrcTextureTransitionMode =
            RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
        copyAttribs.pDstTexture = mSwapChain->GetCurrentBackBufferRTV()->GetTexture();
        copyAttribs.pSrcTexture = mColorBuffer;
        immediateContext->CopyTexture(copyAttribs);

        // Synchronize if necessary
        if (mBackend == GraphicsBackend::OPENGL) {
            // For OpenGL with GLFW, we need to call glfwSwapBuffers.
            mNativeWindowProvider->GLSwapBuffers();
        } else {
            mSwapChain->Present();
        }

        // Read the results of the entity pick requests
        if (bEntityPickEnabled) {
            auto queryCount = mEntityPicker.GetRequestedQueryCount();

            if (queryCount > 0) {
                mEntityPicker.GetResults(
                    immediateContext, 
                    &mEntityResultBuffer[0]);
                for (size_t i = 0; i < queryCount; ++i) {
                    mEntityResultPromises[i].Set(
                        entt::entity(mEntityResultBuffer[i]));
                }
            }
        }
    }

    void BasicRenderer::Fork(core::Frame& frame, 
        core::SyncObject& syncObject,
        const core::Time& time) {

        // Schedule the render
        mRenderFinished.add();
        marl::Task task([this, &frame, renderFinished = mRenderFinished, &syncObject]() {
            defer(renderFinished.done());
            Render(frame, syncObject);
        }, marl::Task::Flags::SameThread);
        marl::schedule(std::move(task));

        // Schedule the updates of resource managers
        mGeometryManager.ScheduleBackend(mRenderFinished);
        mTextureManager.ScheduleBackend(mRenderFinished);
        mBaseMaterialManager.ScheduleBackend(mRenderFinished);
    }

    void BasicRenderer::Join(core::Frame& frame) {
        Wait();
    }

    void BasicRenderer::Wait() {
        mRenderFinished.wait();
    }

    void BasicRenderer::Shutdown() {
        for (auto overlay : mOverlays) {
            overlay->Shutdown();
        }

        mEntityPicker = TextureCapturePick();
        mEntityPickTexture.Release();
        mColorBuffer.Release();
        mDepthBuffer.Release();

        mSceneGlobals = DynamicUniformBuffer<HLSL::SceneGlobals>();
        mInstanceData = DynamicUniformBuffer<HLSL::StaticInstanceData>();
        mStaticMeshPipeline = StaticMeshPipeline();

        mSpriteModule.Shutdown();
        mDefaultTexture.Release();
        mSwapChain.Release();
        mContexts.clear();
        mDevice.Release();
        mEngineFactory.Release();
    }

    void BasicRenderer::AddModule(std::unique_ptr<IGraphicsObject>&&) {
        throw std::runtime_error("Not implemented!");
    }

    void BasicRenderer::AddOverlay(IGraphicsObject* object) {
        auto renderModule = dynamic_cast<IRenderModule*>(object);

        if (renderModule) {
            mOverlays.emplace(renderModule);
        } else {
            throw std::runtime_error("Overlay is not IRenderModule!");
        }
    }

    void BasicRenderer::RemoveOverlay(IGraphicsObject* object) {
        auto renderModule = dynamic_cast<IRenderModule*>(object);

        if (renderModule) {
            mOverlays.erase(renderModule);
        } else {
            throw std::runtime_error("Overlay is not IRenderModule!");
        }
    }
    glm::i32vec2 BasicRenderer::GetRenderArea() const {
        return glm::i32vec2(
            mSwapChain->GetDesc().Width, 
            mSwapChain->GetDesc().Height);
    }

    DynamicUniformBuffer<HLSL::SceneGlobals>*
        BasicRenderer::GetGlobalsBuffer() {
        return &mSceneGlobals;
    }

    core::Future<entt::entity> BasicRenderer::Pick(const glm::vec2& position) {
        core::Promise<entt::entity> promise;
        core::Future<entt::entity> future(promise);
        mEntityPickRequests.ProducerEnqueue(
            EntityPickRequest{position, 
            std::move(promise)});
        return future;
    }
}