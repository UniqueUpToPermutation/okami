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

    struct GeometryImpl {
        std::vector<DG::RefCntAutoPtr<DG::IBuffer>>
            mVertexBuffers;
        DG::RefCntAutoPtr<DG::IBuffer>
            mIndexBuffer;
    };

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
            [this](core::Geometry* geo) { OnDestroy(geo); }) {

        // Associate the renderer with the geometry 
        resources.Register<core::Geometry>(this);
    }

    void BasicRenderer::OnDestroy(core::Geometry* geometry) {
        auto impl = reinterpret_cast<GeometryImpl*>(geometry->GetBackend());
        delete impl;
        delete geometry;
    }

    void BasicRenderer::OnFinalize(core::Geometry* geometry) {
        auto impl = std::make_unique<GeometryImpl>();

        const auto& geoDesc = geometry->Desc();
        auto& data = geometry->DataCPU();

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

        geometry->SetBackend(impl.release());
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

        mStaticMeshVS.Attach(CompileEmbeddedShader(mDevice, paramsStaticMeshVS, &fileLoader, bAddLineNumbers));
        mStaticMeshPS.Attach(CompileEmbeddedShader(mDevice, paramsStaticMeshPS, &fileLoader, bAddLineNumbers));

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

        PSOCreateInfo.pVS = mStaticMeshVS;
        PSOCreateInfo.pPS = mStaticMeshPS;

        DG::IPipelineState* meshPipeline = nullptr;
        mDevice->CreateGraphicsPipelineState(PSOCreateInfo, &meshPipeline);
        mStaticMeshPipeline.Attach(meshPipeline);
    }

    void BasicRenderer::RegisterInterfaces(core::InterfaceCollection& interfaces) {
        interfaces.Add<core::IVertexLayoutProvider>(this);
    }

    void BasicRenderer::LoadResources(core::Frame* frame, 
        marl::WaitGroup& waitGroup) {

        waitGroup.add();
        marl::Task task([&geoManager = mGeometryManager, waitGroup]() {
            defer(waitGroup.done());
            geoManager.RunBackend(true);
        }, marl::Task::Flags::SameThread);

        marl::schedule(std::move(task));
    }

    core::Handle<core::Geometry> BasicRenderer::Load(
        const std::filesystem::path& path, 
        const core::LoadParams<core::Geometry>& params, 
        core::resource_id_t newResId) {

        auto loader = [](const std::filesystem::path& path, 
            const core::LoadParams<core::Geometry>& geo) {
            return core::Geometry::Load(path, geo.mLayout);
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

        // Schedule the update of the geometry manager
        updateGroup.add();
        marl::Task managerUpdates([geoManager = &mGeometryManager, updateGroup] {
            defer(updateGroup.done());
            geoManager->RunBackend();
        }, marl::Task::Flags::SameThread);
        marl::schedule(std::move(managerUpdates));

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

            auto geometryImpl = reinterpret_cast<GeometryImpl*>(mesh.mGeometry->GetBackend());

            DG::IBuffer* vertBuffers[] = { geometryImpl->mVertexBuffers[0] };
            DG::Uint64 offsets[] = { 0 };
            immediateContext->SetVertexBuffers(0, 1, vertBuffers, offsets, 
                RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_NONE);
            if (geometryImpl->mIndexBuffer) {
                immediateContext->SetIndexBuffer(geometryImpl->mIndexBuffer, 0, 
                    RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            }

            immediateContext->SetPipelineState(mStaticMeshPipeline);
            
            const auto& desc = mesh.mGeometry->Desc();

            if (geometryImpl->mIndexBuffer) {
                DG::DrawIndexedAttribs attribs;
                attribs.NumIndices = desc.mIndexedAttribs.mNumIndices;
                attribs.IndexType = ToDiligent(desc.mIndexedAttribs.mIndexType);
                immediateContext->DrawIndexed(attribs);
            } else {
                DG::DrawAttribs attribs;
                attribs.NumVertices = desc.mAttribs.mNumVertices;
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
        mStaticMeshPipeline.Release();
        mStaticMeshPS.Release();
        mStaticMeshVS.Release();

        mSwapChain.Release();
        mContexts.clear();
        mDevice.Release();
        mEngineFactory.Release();
    }

    std::unique_ptr<core::ISystem> CreateRenderer(core::ISystem* displaySystem, core::ResourceInterface& resources) {
        return std::make_unique<BasicRenderer>(displaySystem, resources);
    }
}