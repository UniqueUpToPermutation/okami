
#include <okami/Transform.hpp>
#include <okami/GraphicsComponents.hpp>
#include <okami/Embed.hpp>

#include <okami/diligent/BasicRenderer.hpp>
#include <okami/diligent/Glfw.hpp>
#include <okami/diligent/Shader.hpp>
#include <okami/diligent/GraphicsUtils.hpp>

using namespace Diligent;

// Load shader locations in TEXT into a lookup
void MakeShaderMap(okami::core::file_map_t* map);

namespace okami::graphics::diligent {

    BasicRenderer::BasicRenderer(
        IDisplay* display,
        core::ResourceInterface& resources) : 
        mDisplay(display),
        mGeometryManager(
            []() { return core::Geometry(); },
            [this](WeakHandle<core::Geometry> geo) { OnDestroy(geo); }),
        mTextureManager(
            []() { return core::Texture(); },
            [this](WeakHandle<core::Texture> tex) { OnDestroy(tex); }),
        mRenderCanvasManager(
            []() -> RenderCanvas { throw std::runtime_error("Error!"); },
            [this](WeakHandle<RenderCanvas> canvas) { OnDestroy(canvas); }),
        mResourceInterface(resources) {

        // Associate the renderer with the geometry 
        resources.Register<core::Geometry>(this);
        resources.Register<core::Texture>(this);
        resources.Register<RenderCanvas>(this);
    }

    void BasicRenderer::OnDestroy(WeakHandle<core::Geometry> geometry) {
        auto impl = reinterpret_cast<GeometryImpl*>(geometry->GetBackend());
        delete impl;
        geometry.Free();
    }

    std::unique_ptr<GeometryImpl>
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

    void BasicRenderer::OnFinalize(WeakHandle<core::Geometry> geometry) {
        auto impl = MoveToGPU(*geometry);
        geometry->DeallocCPU();
        geometry->SetBackend(impl.release());
    }

    void BasicRenderer::OnDestroy(WeakHandle<core::Texture> tex) {
        auto impl = GetTextureImpl(tex);
        delete impl;
        tex.Free();
    }

    void BasicRenderer::OnFinalize(WeakHandle<RenderCanvas> canvas) {
        UpdateFramebuffer(canvas);
    }

    inline BasicRenderer::RenderCanvasImpl* GetRenderCanvasImpl(RenderCanvas* canvas) {
        return reinterpret_cast<BasicRenderer::RenderCanvasImpl*>(canvas->GetBackend());
    }

    void BasicRenderer::OnDestroy(WeakHandle<RenderCanvas> canvas) {
        auto impl = GetRenderCanvasImpl(canvas);
        delete impl;
        canvas.Free();
    }

    void BasicRenderer::UpdateFramebuffer(RenderCanvas* canvas) {
        auto impl = GetRenderCanvasImpl(canvas);

        const auto& pass = canvas->GetPassInfo();
        bool bWindowBackend = false;
        bool bForceResize = false;

        if (!impl) {
            // Create the backend
            impl = new RenderCanvasImpl();
            
            auto window = canvas->GetWindow();

            if (window) {
                if (!pass.IsFinal()) {
                    throw std::runtime_error("All passes that write to "
                        "a RenderCanvas backended by a window must be final!");
                }

                DG::ISwapChain* swapChain = nullptr;
                CreateSwapChain(mDevice, mContexts[0], &swapChain,
                    mDefaultSCDesc, mEngineFactory, window);

                const auto& scDesc = swapChain->GetDesc();
                if (mColorFormat != scDesc.ColorBufferFormat) {
                    throw std::runtime_error("Swap chain has improper color format!");
                }

                impl->mSwapChain = swapChain;
                bWindowBackend = true;
            }

            impl->mRenderTargets.resize(pass.mAttributeCount);
            bForceResize = true;
            canvas->SetBackend(impl);
        } 
        
        if (!bWindowBackend) {
            // Update if necessary
            auto size = canvas->GetSize();

            if (size.bHasResized || bForceResize) {
                for (auto& rt : impl->mRenderTargets) {
                    if (rt)
                        rt->Release();
                }

                if (impl->mDepthTarget)
                    impl->mDepthTarget->Release();

                for (uint i = 0; i < pass.mAttributeCount; ++i) {
                    TextureDesc dg_desc;
                    dg_desc.BindFlags = BIND_RENDER_TARGET;
                    dg_desc.CPUAccessFlags = CPU_ACCESS_NONE;
                    dg_desc.Width = size.mWidth;
                    dg_desc.Height = size.mHeight;
                    dg_desc.Type = RESOURCE_DIM_TEX_2D;
                    dg_desc.SampleCount = 1;
                    dg_desc.MipLevels = 1;
                    dg_desc.Name = "Render Canvas RT Backend";
                    dg_desc.Format = 
                        GetFormat(pass.mAttributes[i]);

                    DG::ITexture* tex = nullptr;
                    mDevice->CreateTexture(dg_desc, nullptr, &tex);
                    impl->mRenderTargets[i].Attach(tex);
                }

                TextureDesc dg_desc;
                dg_desc.BindFlags = BIND_RENDER_TARGET;
                dg_desc.CPUAccessFlags = CPU_ACCESS_NONE;
                dg_desc.Width = size.mWidth;
                dg_desc.Height = size.mHeight;
                dg_desc.Type = RESOURCE_DIM_TEX_2D;
                dg_desc.SampleCount = 1;
                dg_desc.MipLevels = 1;
                dg_desc.Name = "Render Canvas DS Backend";
                dg_desc.Format = 
                    GetDepthFormat(pass);

                DG::ITexture* tex = nullptr;
                mDevice->CreateTexture(dg_desc, nullptr, &tex);
                impl->mDepthTarget.Attach(tex);

                canvas->MakeClean();
            }
        }
    }

    std::unique_ptr<TextureImpl> 
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

    void BasicRenderer::OnFinalize(WeakHandle<core::Texture> texture) {
        auto impl = MoveToGPU(*texture);
        texture->DeallocCPU();
        texture->SetBackend(impl.release());
    }

    void BasicRenderer::Startup(marl::WaitGroup& waitGroup) {
        // Load shaders
        core::EmbeddedFileLoader fileLoader(&MakeShaderMap);

        // Create the graphics device and swap chain
        DG::IEngineFactory* factory = nullptr;
        DG::IRenderDevice* device  = nullptr;
        std::vector<DG::IDeviceContext*> contexts;

        if (mDisplay->GetRequestedBackend() == GraphicsBackend::VULKAN) {
            mColorFormat = DG::TEX_FORMAT_BGRA8_UNORM_SRGB;
        } else {
            mColorFormat = DG::TEX_FORMAT_RGBA8_UNORM_SRGB;
        }
        mDepthFormat = DG::TEX_FORMAT_D32_FLOAT;

        auto getInitializationAttribs = [this, 
            colorFormat = mColorFormat,
            depthFormat = mDepthFormat](
            DG::RENDER_DEVICE_TYPE DeviceType, 
            DG::EngineCreateInfo& EngineCI, 
            DG::SwapChainDesc& scDesc) {

            scDesc.ColorBufferFormat            = colorFormat;
            scDesc.DepthBufferFormat            = depthFormat;
            EngineCI.Features.GeometryShaders   = DEVICE_FEATURE_STATE_ENABLED;
            EngineCI.Features.Tessellation 		= DEVICE_FEATURE_STATE_ENABLED;
        };

        CreateDevice(mDisplay->GetRequestedBackend(), 
            &factory, &device, contexts, &mDefaultSCDesc, 
            getInitializationAttribs);

        mEngineFactory.Attach(factory);
        mDevice.Attach(device);

        for (auto context : contexts) {
            mContexts.emplace_back(
                DG::RefCntAutoPtr<DG::IDeviceContext>(context));
        }

        // Create the default texture
        const uint defaultTexWidth = 16;
        const uint defaultTexHeight = 16;

        DG::TextureDesc defaultTexDesc;
        defaultTexDesc.BindFlags = DG::BIND_SHADER_RESOURCE;
        defaultTexDesc.Format = DG::TEX_FORMAT_RGBA8_UNORM_SRGB;
        defaultTexDesc.Width = defaultTexWidth;
        defaultTexDesc.Height = defaultTexHeight;
        defaultTexDesc.MipLevels = 1;
        defaultTexDesc.Type = DG::RESOURCE_DIM_TEX_2D;
        
        std::vector<uint8_t> defaultTexDataRaw(4 * defaultTexWidth * defaultTexHeight);
        std::fill(defaultTexDataRaw.begin(), defaultTexDataRaw.end(), 255u);

        std::vector<DG::TextureSubResData> subData(1);
        subData[0].Stride = 4 * defaultTexWidth;
        subData[0].pData = defaultTexDataRaw.data();

        DG::TextureData defaultTexData;
        defaultTexData.NumSubresources = 1;
        defaultTexData.pContext = mContexts[0];
        defaultTexData.pSubResources = subData.data();

        DG::ITexture* defaultTexture = nullptr;
        mDevice->CreateTexture(defaultTexDesc, &defaultTexData, &defaultTexture);
        mDefaultTexture.Attach(defaultTexture);

        // Create globals buffer
        mSceneGlobals = DynamicUniformBuffer<HLSL::SceneGlobals>(mDevice);

        AddModule(std::make_unique<StaticMeshModule>());

        // This should spawn requested render canvas backends.
        mRenderCanvasManager.RunBackend(true);

        mColorPass = RenderPass::Final();

        RenderModuleParams params;
        params.mFileSystem = &fileLoader;
        params.mDefaultTexture = mDefaultTexture;
        params.mRequestedRenderPasses = {
            mColorPass,
        };

        for (auto& module : mRenderModules) {
            module->Startup(this, mDevice, params);
        }

        for (auto overlay : mOverlays) {
            overlay->Startup(this, mDevice, params);
        }

        for (auto& module : mRenderModules) {
            module->RegisterResourceInterfaces(mResourceInterface);
            module->RegisterVertexFormats(mVertexLayouts);
        }
    }

    void BasicRenderer::RegisterInterfaces(core::InterfaceCollection& interfaces) {
        interfaces.Add<core::IVertexLayoutProvider>(this);
        interfaces.Add<IRenderer>(this);
        interfaces.Add<IGlobalsBufferProvider>(this);
        interfaces.Add<IRenderPassFormatProvider>(this);
    }

    void BasicRenderer::LoadResources(marl::WaitGroup& waitGroup) {
        mGeometryManager.ScheduleBackend(waitGroup, true);
        mTextureManager.ScheduleBackend(waitGroup, true);
        mRenderCanvasManager.ScheduleBackend(waitGroup, true);

        for (auto& module : mRenderModules) { 
            module->Update(true);
        }
    }

    void BasicRenderer::SetFrame(core::Frame& frame) {
    }

    Handle<core::Geometry> BasicRenderer::Load(
        const std::filesystem::path& path, 
        const core::LoadParams<core::Geometry>& params, 
        resource_id_t newResId) {

        auto loader = [this](const std::filesystem::path& path, 
            const core::LoadParams<core::Geometry>& params) {
            auto layout = GetVertexLayout(params.mComponentType);
            return core::Geometry::Load(path, layout);
        };

        auto finalizer = [this](WeakHandle<core::Geometry> geo) {
            OnFinalize(geo);
        };

        return mGeometryManager.Load(path, params, newResId, loader, finalizer);
    } 

    Handle<core::Geometry> BasicRenderer::Add(core::Geometry&& obj, 
        resource_id_t newResId) {
        auto finalizer = [this](WeakHandle<core::Geometry> geo) {
            OnFinalize(geo);
        };
        return mGeometryManager.Add(std::move(obj), newResId, finalizer);
    }

    Handle<core::Geometry> BasicRenderer::Add(core::Geometry&& obj, 
        const std::filesystem::path& path, 
        resource_id_t newResId) {
        auto finalizer = [this](WeakHandle<core::Geometry> geo) {
            OnFinalize(geo);
        };
        return mGeometryManager.Add(std::move(obj), path, newResId, finalizer);
    }

    Handle<core::Texture> BasicRenderer::Load(
        const std::filesystem::path& path, 
        const core::LoadParams<core::Texture>& params, 
        resource_id_t newResId) {
        auto loader = [](const std::filesystem::path& path, 
            const core::LoadParams<core::Texture>& params) {
            return core::Texture::Load(path, params);
        };

        auto finalizer = [this](WeakHandle<core::Texture> tex) {
            OnFinalize(tex);
        };

        return mTextureManager.Load(path, params, newResId, loader, finalizer);
    }

    Handle<core::Texture> BasicRenderer::Add(core::Texture&& obj, 
        resource_id_t newResId) {
        auto finalizer = [this](WeakHandle<core::Texture> tex) {
            OnFinalize(tex);
        };
        return mTextureManager.Add(std::move(obj), newResId, finalizer);
    }

    Handle<core::Texture> BasicRenderer::Add(core::Texture&& obj, 
        const std::filesystem::path& path, 
        resource_id_t newResId) {
        auto finalizer = [this](WeakHandle<core::Texture> tex) {
            OnFinalize(tex);
        };
        return mTextureManager.Add(std::move(obj), path, newResId, finalizer);
    }

    Handle<RenderCanvas> BasicRenderer::Load(
        const std::filesystem::path& path, 
        const core::LoadParams<RenderCanvas>& params, 
        resource_id_t newResId) {
        throw std::runtime_error("Not implemented!");
    }

    Handle<RenderCanvas> BasicRenderer::Add(RenderCanvas&& obj, 
        resource_id_t newResId) {
        auto finalizer = [this](WeakHandle<RenderCanvas> canvas) {
            OnFinalize(canvas);
        };
        return mRenderCanvasManager.Add(std::move(obj), newResId, finalizer);
    }

    Handle<RenderCanvas> BasicRenderer::Add(RenderCanvas&& obj, 
        const std::filesystem::path& path, 
        resource_id_t newResId) {
        throw std::runtime_error("Not implemented!");
    }

    const core::VertexFormat& BasicRenderer::GetVertexLayout(
        const entt::meta_type& type) const {
        return mVertexLayouts.Get(type);
    }

    void BasicRenderer::RequestSync(core::SyncObject& syncObject) {
    }

    void BasicRenderer::Render(core::Frame& frame,
        const std::vector<RenderView>& views,
        core::SyncObject& syncObject,
        const core::Time& time) {

        // Schedule the updates of resource managers
        mRenderCanvasManager.RunBackend();

        marl::WaitGroup resourceWait;
        mGeometryManager.ScheduleBackend(resourceWait);
        mTextureManager.ScheduleBackend(resourceWait);

        for (auto& module : mRenderModules) { 
            module->Update(false);
        }

        syncObject.WaitUntilFinished<core::Camera>();

        auto& registry = frame.Registry();

        std::vector<RenderView> viewsSorted(views);
        std::sort(viewsSorted.begin(), viewsSorted.end(),
            [](const RenderView& rv1, const RenderView& rv2) {
            uint rv1_i = rv1.mTarget->GetWindow() == nullptr ? 0 : 1;
            uint rv2_i = rv2.mTarget->GetWindow() == nullptr ? 0 : 1;
            return rv1_i < rv2_i;
        });

        for (auto rv : views) {
            uint width = rv.mTarget->GetWidth();
            uint height = rv.mTarget->GetHeight();

            // Queue up render calls and compute transforms
            HLSL::SceneGlobals shaderGlobals;
            shaderGlobals.mCamera.mView = DG::float4x4::Identity();
            shaderGlobals.mCamera.mViewProj = DG::float4x4::Identity();
            shaderGlobals.mCamera.mProj = DG::float4x4::Identity();
            shaderGlobals.mCamera.mViewport = DG::float2(width, height);
            shaderGlobals.mCamera.mNearZ = 0.0;
            shaderGlobals.mCamera.mFarZ = 1.0;

            RenderModuleGlobals rmGlobals;
            rmGlobals.mProjection = DG::float4x4::Identity();
            rmGlobals.mView = DG::float4x4::Identity();
            rmGlobals.mViewportSize = DG::float2(width, height);
            rmGlobals.mTime = time;
            rmGlobals.mWorldUp = DG::float3(0.0f, 1.0f, 0.0f);

            rmGlobals.mViewOrigin = DG::float3(0.0f, 0.0f, 0.0f);
            rmGlobals.mViewDirection = DG::float3(0.0f, 0.0f, 1.0f);

            auto cameraEntity = rv.mCamera;
            core::Camera camera;
            core::Camera* cameraPtr = nullptr;

            if (cameraEntity != entt::null) {
                camera = registry.get<core::Camera>(cameraEntity);
                cameraPtr = &camera;
            }
            
            auto renderCanvas = GetRenderCanvasImpl(rv.mTarget);
            DG::SwapChainDesc scDesc;
            if (renderCanvas->mSwapChain) {
                scDesc = renderCanvas->mSwapChain->GetDesc();
            } else {
                scDesc.Width = width;
                scDesc.Height = height;
                scDesc.PreTransform = ToDiligent(rv.mTarget->GetSurfaceTransform());
            }

            rmGlobals.mProjection = GetProjection(cameraPtr, scDesc, false);
            
            core::Transform* transform = nullptr;
            if (cameraEntity != entt::null) {
                transform = registry.try_get<core::Transform>(cameraEntity);
                if (transform) {
                    rmGlobals.mView = ToMatrix(*transform).Inverse();
                }
            }

            shaderGlobals.mCamera.mView = rmGlobals.mView;
            shaderGlobals.mCamera.mProj = rmGlobals.mProjection;
            shaderGlobals.mCamera.mViewProj = rmGlobals.mView * rmGlobals.mProjection;
            shaderGlobals.mCamera.mNearZ = camera.mNearPlane;
            shaderGlobals.mCamera.mFarZ = camera.mFarPlane;

            if (transform) {
                rmGlobals.mViewOrigin = ToDiligent(
                    transform->ApplyToPoint(glm::vec3(0.0f, 0.0f, 0.0f)));
                rmGlobals.mViewDirection = ToDiligent(
                    transform->ApplyToTangent(glm::vec3(0.0f, 0.0f, 1.0f)));
            }

            rmGlobals.mCamera = camera;

            shaderGlobals.mCamera.mInvView = shaderGlobals.mCamera.mView.Inverse();
            shaderGlobals.mCamera.mInvViewProj = shaderGlobals.mCamera.mViewProj.Inverse();
            shaderGlobals.mCamera.mInvProj = shaderGlobals.mCamera.mProj.Inverse();
        
            auto immediateContext = mContexts[0];

            auto canvasImpl = reinterpret_cast<RenderCanvasImpl*>(rv.mTarget->GetBackend());
            if (canvasImpl) {
                const auto& pass = rv.mTarget->GetPassInfo();
                std::vector<ITextureView*> rtvs(pass.mAttributeCount);
                ITextureView* dsv = nullptr;

                if (!canvasImpl->mSwapChain) {
                    for (uint i = 0; i < pass.mAttributeCount; ++i) {
                        rtvs[i] = canvasImpl->mRenderTargets[i]
                            ->GetDefaultView(DG::TEXTURE_VIEW_RENDER_TARGET);
                    }

                    dsv = canvasImpl->mDepthTarget->GetDefaultView(
                        DG::TEXTURE_VIEW_DEPTH_STENCIL);
                } else {
                    rtvs[0] = canvasImpl->mSwapChain->GetCurrentBackBufferRTV();
                    dsv = canvasImpl->mSwapChain->GetDepthBufferDSV();
                }

                immediateContext->SetRenderTargets(rtvs.size(), rtvs.data(), dsv, 
                    RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

                // Move globals to GPU
                mSceneGlobals.Write(immediateContext, shaderGlobals);

                // Clear screen
                if (rv.bClear) {
                    float color[] = { 0.3f, 0.3f, 0.3f, 1.0f };
                    for (uint i = 0; i < pass.mAttributeCount; ++i) {
                        immediateContext->ClearRenderTarget(rtvs[i], 
                            color, 
                            RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
                    }
                    immediateContext->ClearDepthStencil(dsv,
                        DG::CLEAR_DEPTH_FLAG, 1.0f, 0, 
                        DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
                }

                for (auto& module : mRenderModules) {
                    module->WaitUntilReady(syncObject);
                    module->QueueCommands(immediateContext,
                        frame,
                        rv,
                        pass,
                        rmGlobals);
                }

                for (auto overlayIt = rv.mTarget->GetOverlaysBegin();
                    overlayIt != rv.mTarget->GetOverlaysEnd();
                    ++overlayIt) {
                    
                    auto overlayImpl = 
                        reinterpret_cast<IRenderModule*>(
                            (*overlayIt)->GetUserData());

                    overlayImpl->QueueCommands(immediateContext,
                        frame,
                        rv,
                        pass,
                        rmGlobals);
                }
            }
        }

        // Synchronize if necessary
        for (auto rv : views) {
            auto canvasImpl = reinterpret_cast<RenderCanvasImpl*>(
                rv.mTarget->GetBackend());

            if (canvasImpl->mSwapChain) {
                canvasImpl->mSwapChain->Present();
            }
        }

        resourceWait.wait();
    }

    void BasicRenderer::Fork(core::Frame& frame, 
        core::SyncObject& syncObject,
        const core::Time& time) {

        // Schedule the render
        mRenderFinished.add();
        marl::Task task([this, 
            &views = mRenderViews,
            &frame, 
            renderFinished = mRenderFinished, 
            &syncObject,
            time]() {
            defer(renderFinished.done());
            Render(frame, views, syncObject, time);
        }, marl::Task::Flags::SameThread);
        marl::schedule(std::move(task));
    }

    void BasicRenderer::Join(core::Frame& frame) {
        Wait();
    }

    void BasicRenderer::Wait() {
        mRenderFinished.wait();
    }

    void BasicRenderer::Shutdown() {

        for (auto& module : mRenderModules) {
            module->Shutdown();
        }

        for (auto overlay : mOverlays) {
            overlay->Shutdown();
        }

        mGeometryManager.Shutdown();
        mTextureManager.Shutdown();
        mRenderCanvasManager.Shutdown();

        mSceneGlobals = DynamicUniformBuffer<HLSL::SceneGlobals>();

        mDefaultTexture.Release();
        mContexts.clear();
        mDevice.Release();
        mEngineFactory.Release();
    }

    void BasicRenderer::AddModule(std::unique_ptr<IGraphicsObject>&& object) {
        auto cast = dynamic_cast<IRenderModule*>(object.get());

        if (cast) {
            std::unique_ptr<IRenderModule> modul(cast);
            object.release();

            mRenderModules.emplace(std::move(modul));
        } else {
            throw std::runtime_error("Module is not IRenderModule!");
        }
    }

    void BasicRenderer::AddOverlay(IGraphicsObject* object) {
        auto renderModule = dynamic_cast<IOverlayModule*>(object);

        if (renderModule) {
            mOverlays.emplace(renderModule);
        } else {
            throw std::runtime_error("Overlay is not IRenderModule!");
        }
    }

    void BasicRenderer::RemoveOverlay(IGraphicsObject* object) {
        auto renderModule = dynamic_cast<IOverlayModule*>(object);

        if (renderModule) {
            mOverlays.erase(renderModule);
        } else {
            throw std::runtime_error("Overlay is not IRenderModule!");
        }
    }

    DG::TEXTURE_FORMAT BasicRenderer::GetFormat(
        RenderAttribute attrib) {
        switch (attrib) {
            case RenderAttribute::COLOR:
                return mColorFormat;
            case RenderAttribute::ENTITY_ID:
                return DG::TEX_FORMAT_R32_UINT;
            default:
                return DG::TEX_FORMAT_UNKNOWN;
        }
    }

    DG::TEXTURE_FORMAT BasicRenderer::GetDepthFormat(
        const RenderPass& pass) {
        return DG::TEX_FORMAT_D32_FLOAT;
    }

    DynamicUniformBuffer<HLSL::SceneGlobals>*
        BasicRenderer::GetGlobalsBuffer() {
        return &mSceneGlobals;
    }

    void BasicRenderer::SetRenderViews(std::vector<RenderView>&& rvs) {
        mRenderViews = std::move(rvs);
    }
}