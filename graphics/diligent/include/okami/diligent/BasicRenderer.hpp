#pragma once

#include <okami/System.hpp>
#include <okami/ResourceManager.hpp>
#include <okami/ResourceInterface.hpp>
#include <okami/Geometry.hpp>
#include <okami/Texture.hpp>
#include <okami/Material.hpp>
#include <okami/Graphics.hpp>

#include <okami/diligent/Glfw.hpp>
#include <okami/diligent/Buffers.hpp>
#include <okami/diligent/SpriteModule.hpp>
#include <okami/diligent/RenderModule.hpp>
#include <okami/diligent/SceneGlobals.hpp>
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
        public core::IResourceManager<core::Geometry>,
        public core::IResourceManager<core::Texture>,
        public core::IResourceManager<RenderCanvas>,
        public core::IVertexLayoutProvider,
        public IRenderer,
        public IGlobalsBufferProvider {
    public:
        struct RenderCanvasImpl {
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

        RenderPass                                  mColorPass;
        RenderPass                                  mDepthPass;
        RenderPass                                  mEntityIdPass;

        DG::TEXTURE_FORMAT                          mColorFormat = DG::TEX_FORMAT_UNKNOWN;

        std::vector<RenderView>                     mRenderViews;
        
        DG::SwapChainDesc                           mDefaultSCDesc;

        core::VertexLayoutRegistry                  mVertexLayouts;
        IDisplay*                                   mDisplay = nullptr;

        GraphicsBackend                             mBackend;
        marl::WaitGroup                             mRenderFinished;
    
        DG::RefCntAutoPtr<DG::ITexture>             mDefaultTexture;

        std::set<IOverlayModule*>                   mOverlays;
        std::set<std::unique_ptr<IRenderModule>>    mRenderModules;

        core::ResourceManager<core::Geometry>       mGeometryManager;
        core::ResourceManager<core::Texture>        mTextureManager;
        core::ResourceManager<RenderCanvas>         mRenderCanvasManager;

        DynamicUniformBuffer<
            HLSL::SceneGlobals>                     mSceneGlobals;

        std::unique_ptr<GeometryImpl>       MoveToGPU(const core::Geometry& geometry);
        std::unique_ptr<TextureImpl>        MoveToGPU(const core::Texture& texture);

        void UpdateFramebuffer(RenderCanvas* canvas);

    public:
        BasicRenderer(
            IDisplay* display,
            core::ResourceInterface& resources);

        void Render(core::Frame& frame,
            const std::vector<RenderView>& views,
            core::SyncObject& syncObject,
            const core::Time& time);

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
        void OnFinalize(core::Geometry* geometry);
        void OnDestroy(core::Geometry* geometry);
        Handle<core::Geometry> Load(
            const std::filesystem::path& path, 
            const core::LoadParams<core::Geometry>& params, 
            resource_id_t newResId) override;
        Handle<core::Geometry> Add(core::Geometry&& obj, 
            resource_id_t newResId) override;
        Handle<core::Geometry> Add(core::Geometry&& obj, 
            const std::filesystem::path& path, 
            resource_id_t newResId) override;

        // Texture resource handlers
        void OnFinalize(core::Texture* texture);
        void OnDestroy(core::Texture* texture);
        Handle<core::Texture> Load(
            const std::filesystem::path& path, 
            const core::LoadParams<core::Texture>& params, 
            resource_id_t newResId) override;
        Handle<core::Texture> Add(core::Texture&& obj, 
            resource_id_t newResId) override;
        Handle<core::Texture> Add(core::Texture&& obj, 
            const std::filesystem::path& path, 
            resource_id_t newResId) override;

        // Texture resource handlers
        void OnFinalize(RenderCanvas* canvas);
        void OnDestroy(RenderCanvas* canvas);
        Handle<RenderCanvas> Load(
            const std::filesystem::path& path, 
            const core::LoadParams<RenderCanvas>& params, 
            resource_id_t newResId) override;
        Handle<RenderCanvas> Add(RenderCanvas&& obj, 
            resource_id_t newResId) override;
        Handle<RenderCanvas> Add(RenderCanvas&& obj, 
            const std::filesystem::path& path, 
            resource_id_t newResId) override;

        void AddModule(std::unique_ptr<IGraphicsObject>&&) override;
        void AddOverlay(IGraphicsObject* object) override;
        void RemoveOverlay(IGraphicsObject* object) override;

        core::TextureFormat GetFormat(RenderAttribute attrib) override;
        core::TextureFormat GetDepthFormat(const RenderPass& pass) override;

        DynamicUniformBuffer<HLSL::SceneGlobals>*
            GetGlobalsBuffer() override;

        void SetRenderViews(std::vector<RenderView>&& rvs) override;
    };
}