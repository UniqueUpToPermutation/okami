#include <okami/BasicRenderer.hpp>
#include <okami/Display.hpp>
#include <okami/Transform.hpp>
#include <okami/GraphicsUtils.hpp>

using namespace Diligent;

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

    BasicRenderer::BasicRenderer(core::ISystem* displaySystem) : 
        mDisplaySystem(displaySystem), 
        mGeometryManager([this](core::Geometry* geo) { OnDestroy(geo); }) {
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
    }

    void BasicRenderer::RegisterInterfaces(core::InterfaceCollection& interfaces) {
    }

    void BasicRenderer::LoadResources(core::Frame* frame, 
        marl::WaitGroup& waitGroup) {
    }

    void BasicRenderer::RequestSync(core::SyncObject& syncObject) {
        syncObject.Read<core::Transform>().add();
    }

    core::Future<core::Handle<core::Geometry>> BasicRenderer::Load(
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

    core::Future<core::Handle<core::Geometry>> BasicRenderer::Add(core::Geometry&& obj, 
        core::resource_id_t newResId) {
        auto finalizer = [this](core::Geometry* geo) {
            OnFinalize(geo);
        };
        return mGeometryManager.Add(std::move(obj), newResId, finalizer);
    }

    core::Future<core::Handle<core::Geometry>> BasicRenderer::Add(core::Geometry&& obj, 
        const std::filesystem::path& path, 
        core::resource_id_t newResId) {
         auto finalizer = [this](core::Geometry* geo) {
            OnFinalize(geo);
        };
        return mGeometryManager.Add(std::move(obj), path, newResId, finalizer);
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

        {
            // Read updated transforms
            syncObject.Read<core::Transform>().done();
        }

        auto backBufferRTV = mSwapChain->GetCurrentBackBufferRTV();
        auto depthBufferDSV = mSwapChain->GetDepthBufferDSV();
        auto immediateContext = mContexts[0];

        ITextureView* backBuffers[] = {backBufferRTV};
        float color[] = { 0.5f, 0.5f, 0.5f, 1.0f };

        immediateContext->SetRenderTargets(1, backBuffers, depthBufferDSV, 
            RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        immediateContext->ClearRenderTarget(backBufferRTV, color, 
            RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

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
        mSwapChain.Release();
        mContexts.clear();
        mDevice.Release();
        mEngineFactory.Release();
    }

    std::unique_ptr<core::ISystem> CreateRenderer(core::ISystem* displaySystem) {
        return std::make_unique<BasicRenderer>(displaySystem);
    }
}