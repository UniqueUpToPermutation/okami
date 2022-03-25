#pragma once

#include <okami/System.hpp>
#include <okami/ResourceManager.hpp>
#include <okami/ResourceBackend.hpp>
#include <okami/Geometry.hpp>
#include <okami/Texture.hpp>
#include <okami/Material.hpp>
#include <okami/Graphics.hpp>

#include <okami/diligent/Glfw.hpp>
#include <okami/diligent/Buffers.hpp>
#include <okami/diligent/SpriteModule.hpp>
#include <okami/diligent/RenderModule.hpp>
#include <okami/diligent/ShaderTypes.hpp>
#include <okami/diligent/TextureCapture.hpp>
#include <okami/diligent/StaticMeshModule.hpp>

#include <RenderDevice.h>
#include <SwapChain.h>
#include <DeviceContext.h>
#include <RefCntAutoPtr.hpp>
#include <BasicMath.hpp>

namespace okami::graphics::diligent {
    namespace DG = Diligent;

    struct EntityPickRequest {
        glm::vec2 mPosition;
        core::Promise<entt::entity> mResult;
    };

    class BasicRenderer final : 
        public core::ISystem,
        public core::IVertexLayoutProvider,
        public IRenderer,
        public IGlobalsBufferProvider,
        public IRenderPassFormatProvider {
    public:
        struct RenderCanvasBackend {
            bool bInitialized = false;
            DG::RefCntAutoPtr<DG::ISwapChain>
                mSwapChain;

            std::vector<DG::RefCntAutoPtr<DG::ITexture>>
                mRenderTargets;
            DG::RefCntAutoPtr<DG::ITexture>
                mDepthTarget;
        };
        
    private:
        DG::RefCntAutoPtr<DG::IRenderDevice>        mDevice;
        DG::RefCntAutoPtr<DG::IEngineFactory>       mEngineFactory;
        std::vector<DG::RefCntAutoPtr<DG::IDeviceContext>>   
            mContexts;
        core::ResourceManager&                      mResourceInterface;

        RenderPass                                  mColorPass;
        RenderPass                                  mDepthPass;
        RenderPass                                  mEntityIdPass;

        DG::TEXTURE_FORMAT                          mColorFormat = DG::TEX_FORMAT_UNKNOWN;
        DG::TEXTURE_FORMAT                          mDepthFormat = DG::TEX_FORMAT_UNKNOWN;

        std::vector<RenderView>                     mRenderViews;
        
        DG::SwapChainDesc                           mDefaultSCDesc;

        core::VertexLayoutRegistry                  mVertexLayouts;
        IDisplay*                                   mDisplay = nullptr;

        GraphicsBackend                             mBackend;
        marl::WaitGroup                             mRenderFinished;
    
        DG::RefCntAutoPtr<DG::ITexture>             mDefaultTexture;

        std::set<IOverlayModule*>                   mOverlays;
        std::set<std::unique_ptr<IRenderModule>>    mRenderModules;

        core::ResourceBackend<
            core::Geometry, GeometryBackend>        mGeometryBackend;
        core::ResourceBackend<
            core::Texture, TextureBackend>          mTextureBackend;
        core::ResourceBackend<
            RenderCanvas, RenderCanvasBackend>      mRenderCanvasBackend;

        DynamicUniformBuffer<
            HLSL::SceneGlobals>                     mSceneGlobals;

        GeometryBackend     MoveToGPU(const core::Geometry& geometry);
        TextureBackend      MoveToGPU(const core::Texture& texture);

    public:
        BasicRenderer(
            IDisplay* display,
            core::ResourceManager& resources);

        void Render(core::Frame& frame,
            const std::vector<RenderView>& views,
            core::SyncObject& syncObject,
            const core::Time& time);

        void UpdateFramebuffer(
            RenderCanvas& frontend, RenderCanvasBackend& backend);

        void Startup(marl::WaitGroup& waitGroup) override;
        void Shutdown() override;

        void RegisterInterfaces(
            core::InterfaceCollection& interfaces) override;
        void SetFrame(core::Frame& frame) override;
        void LoadResources(marl::WaitGroup& waitGroup) override;
        void RequestSync(core::SyncObject& syncObject) override;
        void Fork(core::Frame& frame,
            core::SyncObject& syncObject,
            const core::Time& time) override;
        void Join(core::Frame& frame) override;
        void Wait() override;

        const core::VertexFormat& GetVertexLayout(
            const entt::meta_type& type) const override;

        // Geometry resource handlers
        void OnFinalize(
            const core::Geometry& geometryIn,
            core::Geometry& geometryOut,
            GeometryBackend& backend);
        void OnDestroy(GeometryBackend& geometry);
        core::Geometry LoadFrontendGeometry(
            const std::filesystem::path& path, 
            const core::LoadParams<core::Geometry>& params);
        GeometryBackend Construct(const core::Geometry& geometry);

        // Texture resource handlers
        void OnFinalize(
            const core::Texture& textureIn,
            core::Texture& textureOut,
            TextureBackend& backend);
        void OnDestroy(TextureBackend& texture);
        TextureBackend Construct(const core::Texture& texture);

        // RenderCanvas resource handlers
        void OnFinalize(
            const RenderCanvas& canvasIn,
            RenderCanvas& canvasOut,
            RenderCanvasBackend& backend);
        void OnDestroy(RenderCanvasBackend& canvas);
        RenderCanvasBackend Construct(const RenderCanvas& canvas);

        void AddModule(std::unique_ptr<IGraphicsObject>&&) override;
        void AddOverlay(IGraphicsObject* object) override;
        void RemoveOverlay(IGraphicsObject* object) override;

        DG::TEXTURE_FORMAT GetFormat(RenderAttribute attrib) override;
        DG::TEXTURE_FORMAT GetDepthFormat(const RenderPass& pass) override;

        DynamicUniformBuffer<HLSL::SceneGlobals>*
            GetGlobalsBuffer() override;

        void SetRenderViews(std::vector<RenderView>&& rvs) override;
    };
}