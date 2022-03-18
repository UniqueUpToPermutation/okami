
#include <okami/Transform.hpp>
#include <okami/GraphicsComponents.hpp>
#include <okami/Embed.hpp>

#include <okami/diligent/BasicRenderer.hpp>
#include <okami/diligent/Glfw.hpp>
#include <okami/diligent/Shader.hpp>
#include <okami/diligent/GraphicsUtils.hpp>

#include <okami/diligent/SpriteModule.hpp>

#include <filesystem>

using namespace Diligent;

// Load shader locations in TEXT into a lookup
void MakeShaderMap(okami::core::file_map_t* map);

namespace okami::graphics::diligent {

    DG::ITexture* GetTextureBackend(TextureBackend* backend) {
        return backend->mTexture.RawPtr();
    };

    BasicRenderer::BasicRenderer(
        IDisplay* display,
        core::ResourceManager& resources) : 
        mDisplay(display),
        mGeometryBackend(
            [this](const core::Geometry& geo) { 
                return Construct(geo); },
            [this](const std::filesystem::path& path, 
                const core::LoadParams<core::Geometry>& params) {
                return LoadFrontendGeometry(path, params); },
            [this](const core::Geometry& geoIn,
                core::Geometry& geoOut,
                GeometryBackend& backend) {
                OnFinalize(geoIn, geoOut, backend); },
            [this](GeometryBackend& backend) {
                OnDestroy(backend); }),
        mTextureBackend(
            [this](const core::Texture& geo) { 
                return Construct(geo); },
            &core::Texture::Load,
            [this](const core::Texture& texIn,
                core::Texture& texOut,
                TextureBackend& backend) {
                OnFinalize(texIn, texOut, backend); },
            [this](TextureBackend& backend) {
                OnDestroy(backend); }),
        mRenderCanvasBackend(
            [this](const RenderCanvas& canvas) {
                return Construct(canvas); },
            nullptr,
            [this](const RenderCanvas& canvIn,
                RenderCanvas& canvOut,
                RenderCanvasBackend& backend) {
                OnFinalize(canvIn, canvOut, backend); },
            [this](RenderCanvasBackend& backend) {
                OnDestroy(backend); }),
        mResourceInterface(resources) {

        // Associate the renderer with the correct resource types 
        resources.Register<core::Geometry>(&mGeometryBackend);
        resources.Register<core::Texture>(&mTextureBackend);
        resources.Register<RenderCanvas>(&mRenderCanvasBackend);
    }

    void BasicRenderer::OnDestroy(GeometryBackend& geometry) {
        geometry = GeometryBackend();
    }

   GeometryBackend
        BasicRenderer::MoveToGPU(const core::Geometry& geometry) {
        GeometryBackend result(geometry.GetDesc());

        const auto& geoDesc = result.mDesc;
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

            result.mVertexBuffers.emplace_back(buffer);
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

            result.mIndexBuffer.Attach(buffer);
        }

        return result;
    }

    void BasicRenderer::OnFinalize(
        const core::Geometry& geometryIn,
        core::Geometry& geometryOut,
        GeometryBackend& backend) {

        backend = MoveToGPU(geometryIn);

        geometryOut.DeallocCPU();
        geometryOut.SetDesc(geometryIn.GetDesc());
        geometryOut.SetBoundingBox(geometryIn.GetBoundingBox());

        backend.mEvent->signal();
    }

    void BasicRenderer::OnDestroy(TextureBackend& texture) {
        texture = TextureBackend();
    }

    core::Geometry BasicRenderer::LoadFrontendGeometry(
        const std::filesystem::path& path, 
        const core::LoadParams<core::Geometry>& params) {
        auto& layout = mVertexLayouts.Get(params.mComponentType);
        return core::Geometry::Load(path, layout);
    }

    GeometryBackend BasicRenderer::Construct(const core::Geometry& geo) {
        return GeometryBackend(geo.GetDesc());
    }

    TextureBackend BasicRenderer::Construct(const core::Texture& tex) {
        return TextureBackend();
    }

    void BasicRenderer::OnFinalize(
        const RenderCanvas& canvasIn,
        RenderCanvas& canvasOut,
        RenderCanvasBackend& backend) {
        UpdateFramebuffer(canvasOut, backend);
    }

    void BasicRenderer::OnDestroy(RenderCanvasBackend& canvas) {
        canvas = RenderCanvasBackend();
    }

    BasicRenderer::RenderCanvasBackend 
        BasicRenderer::Construct(const RenderCanvas&) {
        return RenderCanvasBackend();
    }

    void BasicRenderer::UpdateFramebuffer(
        RenderCanvas& frontend, 
        RenderCanvasBackend& backend) {

        const auto& pass = frontend.GetPassInfo();
        bool bWindowBackend = frontend.GetWindow() != nullptr;
        bool bForceResize = false;

        if (!backend.bInitialized) {
            // Create the backend
            auto window = frontend.GetWindow();

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

                backend.mSwapChain = swapChain;
                bWindowBackend = true;
                frontend.SetSurfaceTransform(ToOkami(scDesc.PreTransform));
            }

            backend.mRenderTargets.resize(pass.mAttributeCount);
            bForceResize = true;
            backend.bInitialized = true;
        } 
        
        if (!bWindowBackend) {
            // Update if necessary
            auto props = frontend.GetProperties();

            if (props.bHasResized || bForceResize) {
                for (auto& rt : backend.mRenderTargets) {
                    if (rt)
                        rt->Release();
                }

                if (backend.mDepthTarget)
                    backend.mDepthTarget->Release();

                for (uint i = 0; i < pass.mAttributeCount; ++i) {
                    TextureDesc dg_desc;
                    dg_desc.BindFlags = BIND_RENDER_TARGET;
                    dg_desc.CPUAccessFlags = CPU_ACCESS_NONE;
                    dg_desc.Width = props.mWidth;
                    dg_desc.Height = props.mHeight;
                    dg_desc.Type = RESOURCE_DIM_TEX_2D;
                    dg_desc.SampleCount = 1;
                    dg_desc.MipLevels = 1;
                    dg_desc.Name = "Render Canvas RT Backend";
                    dg_desc.Format = 
                        GetFormat(pass.mAttributes[i]);

                    DG::ITexture* tex = nullptr;
                    mDevice->CreateTexture(dg_desc, nullptr, &tex);
                    backend.mRenderTargets[i].Attach(tex);
                }

                TextureDesc dg_desc;
                dg_desc.BindFlags = BIND_RENDER_TARGET;
                dg_desc.CPUAccessFlags = CPU_ACCESS_NONE;
                dg_desc.Width = props.mWidth;
                dg_desc.Height = props.mHeight;
                dg_desc.Type = RESOURCE_DIM_TEX_2D;
                dg_desc.SampleCount = 1;
                dg_desc.MipLevels = 1;
                dg_desc.Name = "Render Canvas DS Backend";
                dg_desc.Format = 
                    GetDepthFormat(pass);

                DG::ITexture* tex = nullptr;
                mDevice->CreateTexture(dg_desc, nullptr, &tex);
                backend.mDepthTarget.Attach(tex);

                if (props.mTransform == SurfaceTransform::OPTIMAL) {
                    throw std::runtime_error("SurfaceTransform::OPTIMAL is not valid for "
                        "a render canvas that is not backed by a swap chain!");
                }
            }
        } else {
            auto props = frontend.GetProperties();
            backend.mSwapChain->Resize(props.mWidth, props.mHeight, 
                ToDiligent(props.mTransform));
        }

        frontend.MakeClean();
    }

    TextureBackend
        BasicRenderer::MoveToGPU(const core::Texture& texture) {
        TextureBackend result;

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
        result.mTexture.Attach(dg_texture);

        return result;
    }

    void BasicRenderer::OnFinalize(
        const core::Texture& textureIn,
        core::Texture& textureOut,
        TextureBackend& backend) {

        backend = MoveToGPU(textureIn);

        textureOut.DeallocCPU();
        textureOut.SetDesc(textureIn.GetDesc());

        backend.mEvent->signal();
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

        AddModule(std::make_unique<StaticMeshModule>(
            &mGeometryBackend, &mTextureBackend));
        AddModule(std::make_unique<
            SpriteModule<TextureBackend, &GetTextureBackend>>(
                &mTextureBackend));

        // This should spawn requested render canvas backends.
        mRenderCanvasBackend.Run();

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
        
        waitGroup.add();
        marl::Task geoTask([
            &geoBackend = mGeometryBackend,
            waitGroup]() {
            defer(waitGroup.done());

            do {
                geoBackend.Run();
                geoBackend.LoadCounter().wait();
            } while (!geoBackend.IsIdle());
        }, marl::Task::Flags::SameThread);

        waitGroup.add();
        marl::Task texTask([
            &texBackend = mTextureBackend,
            waitGroup]() {
            defer(waitGroup.done());

            do {
                texBackend.Run();
                texBackend.LoadCounter().wait();
            } while (!texBackend.IsIdle());
        }, marl::Task::Flags::SameThread);

        waitGroup.add();
        marl::Task canvTask([
            &canvBackend = mRenderCanvasBackend,
            waitGroup]() {
            defer(waitGroup.done());

            do {
                canvBackend.Run();
                canvBackend.LoadCounter().wait();
            } while (!canvBackend.IsIdle());
        }, marl::Task::Flags::SameThread);

        marl::schedule(std::move(geoTask));
        marl::schedule(std::move(texTask));
        marl::schedule(std::move(canvTask));

        for (auto& module : mRenderModules) {
            waitGroup.add();
            marl::Task modTask([
                &module,
                &resources = mResourceInterface,
                waitGroup]() {
                defer(waitGroup.done());

                do {
                    module->Update(&resources);
                    module->WaitOnPendingTasks();
                } while (!module->IsIdle());
            }, marl::Task::Flags::SameThread);

            marl::schedule(std::move(modTask));
        }
    }

    void BasicRenderer::SetFrame(core::Frame& frame) {
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
        mRenderCanvasBackend.Run();
        mRenderCanvasBackend.ForEach([this, &resources = mResourceInterface]
            (resource_id_t id, RenderCanvasBackend& backend) {
            auto frontend = resources.TryGet<RenderCanvas>(id);
            if (frontend) {
                auto properties = frontend->GetProperties();
                if (properties.bHasResized) {
                    UpdateFramebuffer(*frontend, backend);
                }
            }
        });

        mGeometryBackend.Run();
        mTextureBackend.Run();

        for (auto& module : mRenderModules) { 
            module->Update(&mResourceInterface);
        }

        syncObject.WaitUntilFinished<core::Camera>();

        auto& registry = frame.Registry();

        std::vector<RenderView> viewsSorted(views);
        std::sort(viewsSorted.begin(), viewsSorted.end(),
            [&resources = mResourceInterface](const RenderView& rv1, const RenderView& rv2) {
            auto& target1 = resources.Get<RenderCanvas>(rv1.mTargetId);
            auto& target2 = resources.Get<RenderCanvas>(rv2.mTargetId);

            uint rv1_i = target1.GetWindow() == nullptr ? 0 : 1;
            uint rv2_i = target2.GetWindow() == nullptr ? 0 : 1;
            return rv1_i < rv2_i;
        });

        for (auto rv : views) {
            auto& target = mResourceInterface.Get<RenderCanvas>(rv.mTargetId);
            uint width = target.GetWidth();
            uint height = target.GetHeight();

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
            
            auto& targetBackend = mRenderCanvasBackend.Get(rv.mTargetId);
            DG::SwapChainDesc scDesc;
            if (targetBackend.mSwapChain) {
                scDesc = targetBackend.mSwapChain->GetDesc();
            } else {
                scDesc.Width = width;
                scDesc.Height = height;
                scDesc.PreTransform = ToDiligent(target.GetSurfaceTransform());
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

            const auto& pass = target.GetPassInfo();
            std::vector<ITextureView*> rtvs(pass.mAttributeCount);
            ITextureView* dsv = nullptr;

            if (!targetBackend.mSwapChain) {
                for (uint i = 0; i < pass.mAttributeCount; ++i) {
                    rtvs[i] = targetBackend.mRenderTargets[i]
                        ->GetDefaultView(DG::TEXTURE_VIEW_RENDER_TARGET);
                }

                dsv = targetBackend.mDepthTarget->GetDefaultView(
                    DG::TEXTURE_VIEW_DEPTH_STENCIL);
            } else {
                rtvs[0] = targetBackend.mSwapChain->GetCurrentBackBufferRTV();
                dsv = targetBackend.mSwapChain->GetDepthBufferDSV();
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
                    target,
                    pass,
                    rmGlobals);
            }

            for (auto overlayIt = target.GetOverlaysBegin();
                overlayIt != target.GetOverlaysEnd();
                ++overlayIt) {
                
                auto overlayImpl = 
                    reinterpret_cast<IRenderModule*>(
                        (*overlayIt)->GetUserData());

                overlayImpl->QueueCommands(immediateContext,
                    frame,
                    rv,
                    target,
                    pass,
                    rmGlobals);
            }
        }

        // Synchronize swap chains
        DG::ISwapChain* primarySwapChain = nullptr;
        for (auto rv : views) {
            
            auto& targetBackend = mRenderCanvasBackend.Get(rv.mTargetId);

            if (targetBackend.mSwapChain) {
                const auto& desc = targetBackend.mSwapChain->GetDesc();

                if (desc.IsPrimary) {
                    if (!primarySwapChain) {
                        primarySwapChain = targetBackend.mSwapChain.RawPtr();
                    } else {
                        std::cout << "WARNING: Multiple primary swap chains detected! " <<
                            "This may cause performance degredation!" << std::endl;
                    }
                    
                } else {
                    targetBackend.mSwapChain->Present(0);
                }
            }
        }

        // Present primary chain last
        if (primarySwapChain) {
            primarySwapChain->Present(1);
        }
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

        mGeometryBackend.Shutdown();
        mTextureBackend.Shutdown();
        mRenderCanvasBackend.Shutdown();

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