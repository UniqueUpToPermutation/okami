#pragma once

#include <okami/System.hpp>
#include <okami/Event.hpp>
#include <okami/Resource.hpp>
#include <okami/Input.hpp>
#include <okami/Texture.hpp>

#include <glm/vec2.hpp>

namespace okami::graphics {

    typedef std::function<void()> immedate_callback_t;

    class IRenderCanvasAttachment;
    class IWindow;

    enum class GraphicsBackend {
        VULKAN,
        D3D11,
        D3D12,
        DEFAULT = VULKAN
    };

    constexpr GraphicsBackend DEFAULT_BACKEND = GraphicsBackend::DEFAULT;

    class IGraphicsObject {
    public:
        virtual ~IGraphicsObject() = default;
        virtual void* GetBackend() = 0;
    };

    class IEntityPick {
    public:
        virtual core::Future<entt::entity> Pick(const glm::vec2& position) = 0;
        inline core::Future<entt::entity> Pick(float x, float y) {
            return Pick(glm::vec2(x, y));
        }
    };

    enum class RenderAttribute {
        NONE,
        COLOR,
        DEPTH,
        UV,
        NORMAL,
        ALBEDO,
        ROUGHNESS,
        METALLIC,
        ENTITY_ID
    };

    struct RenderPass {
        static constexpr uint MaxAttributes = 8u;

        std::array<RenderAttribute, MaxAttributes> mAttributes;
        uint mAttributeCount = 0;

        bool operator==(const RenderPass& pass);

        struct Hasher {
            std::size_t operator()(const RenderPass& p) const noexcept;
        };

        inline bool IsFinal() const {
            return mAttributeCount == 1 &&
                mAttributes[0] == RenderAttribute::COLOR;
        }

        inline static RenderPass Final() {
            RenderPass result;
            result.mAttributeCount = 1;
            result.mAttributes[0] = RenderAttribute::COLOR;

            return result;
        }
    };

    enum class SurfaceTransform {
        NONE,
        OPTIMAL,
        IDENTITY,
        ROTATE_90,
        ROTATE_180,
        ROTATE_270,
        HORIZONTAL_MIRROR,
        HORIZONTAL_MIRROR_ROTATE_90,
        HORIZONTAL_MIRROR_ROTATE_180,
        HORIZONTAL_MIRROR_ROTATE_270
    };

    struct RenderCanvasDesc {
        uint mWidth;
        uint mHeight;
        RenderPass mPassInfo;
        IWindow* mNativeWindowInterface = nullptr;
    };

    class RenderCanvas;
    
    class IRenderCanvasAttachment {
    public:
        virtual void OnAttach(RenderCanvas* canvas) = 0;
        virtual void OnDettach(RenderCanvas* canvas) = 0;
        virtual void* GetUserData() = 0;
    };

    struct RenderCanvasSize {
        uint mWidth;
        uint mHeight;
        SurfaceTransform mTransform;
        bool bHasResized;
    };

    class RenderCanvas : public Resource {
    private:
        std::atomic<uint> mWidth;
        std::atomic<uint> mHeight;
        std::atomic<bool> bHasResized;
        RenderPass mPassInfo;
        SurfaceTransform mSurfaceTransform;
        IWindow* mWindow = nullptr;
        std::set<IRenderCanvasAttachment*> mOverlays;

    public:
        inline SurfaceTransform GetSurfaceTransform() const {
            return mSurfaceTransform;
        }

        inline void SetSurfaceTransform(SurfaceTransform transform) {
            mSurfaceTransform = transform;
        }

        RenderCanvas(const RenderCanvas&) = delete;
        RenderCanvas& operator=(const RenderCanvas&) = delete;

        inline RenderCanvas& operator=(RenderCanvas&& other) {
            mWidth.exchange(other.mWidth);
            mHeight.exchange(other.mHeight);
            bHasResized.exchange(other.bHasResized);

            mPassInfo = std::move(other.mPassInfo);
            mSurfaceTransform = std::move(other.mSurfaceTransform);
            mWindow = std::move(other.mWindow);
            mOverlays = std::move(other.mOverlays);

            return *this;
        }

        inline RenderCanvas(RenderCanvas&& other) {
            *this = std::move(other);
        }

        inline RenderCanvasSize GetSize() const {
            return RenderCanvasSize{
                mWidth,
                mHeight,
                mSurfaceTransform,
                bHasResized
            };
        }

        inline void MakeClean() {
            bHasResized = false;
        }

        inline void AddOverlay(IRenderCanvasAttachment* object) {
            mOverlays.emplace(object);
            object->OnAttach(this);
        }

        inline void RemoveOverlay(IRenderCanvasAttachment* object) {
            object->OnDettach(this);
            mOverlays.erase(object);
        }

        inline auto GetOverlaysBegin() {
            return mOverlays.begin();
        }

        inline auto GetOverlaysEnd() {
            return mOverlays.end();
        }

        inline void Resize(uint width, uint height) {
            mWidth = width;
            mHeight = height;
            bHasResized = true;
        }

        inline uint GetWidth() const {
            return mWidth;
        }

        inline uint GetHeight() const {
            return mHeight;
        }

        inline RenderPass GetPassInfo() const {
            return mPassInfo;
        }

        inline IWindow* GetWindow() {
            return mWindow;
        }

        inline RenderCanvas(const RenderCanvasDesc& desc) :
            mWidth(desc.mWidth),
            mHeight(desc.mHeight),
            mPassInfo(desc.mPassInfo) {
        }

        ~RenderCanvas();

        entt::meta_type GetType() const override;
        static void Register();
    };

    struct RenderView {
        Handle<RenderCanvas> mTarget;
        entt::entity mCamera;
        bool bClear = true;
    };

    class IRenderer {
    public:
        virtual void AddModule(std::unique_ptr<IGraphicsObject>&&) = 0;
        virtual void AddOverlay(IGraphicsObject* object) = 0;
        virtual void RemoveOverlay(IGraphicsObject* object) = 0;
        virtual core::TextureFormat GetFormat(RenderAttribute attrib) = 0;
        virtual core::TextureFormat GetDepthFormat(const RenderPass& pass) = 0;
        virtual void SetRenderViews(std::vector<RenderView>&& rvs) = 0;

        inline void SetRenderView(RenderView v) {
            SetRenderViews({ v });
        }
    };

    struct WindowParams {
        constexpr static uint DEFAULT_WINDOW_WIDTH  = 1920u;
        constexpr static uint DEFAULT_WINDOW_HEIGHT = 1080u;

		std::string mWindowTitle 	= "okami";
		uint mBackbufferWidth 	    = DEFAULT_WINDOW_WIDTH;
		uint mBackbufferHeight 		= DEFAULT_WINDOW_HEIGHT;
		bool bFullscreen 			= false;
		bool bShowOnCreation		= true;
        bool bIsPrimary             = true;
	};

    struct RealtimeGraphicsParams {
        GraphicsBackend mBackend = GraphicsBackend::DEFAULT;
    };

    enum class StandardCursor {
        ARROW,
        IBEAM,
        CROSSHAIR,
        HAND,
        HRESIZE,
        VRESIZE,
        RESIZE_ALL,
        RESIZE_NESW,
        RESIZE_NWSE,
        NOT_ALLOWED
    };

    class ICursor {
    public:
        virtual uint GetId() const = 0;
        virtual void* GetBackend() = 0;

        virtual ~ICursor() = default;
    };

    class IWindow : public core::IInputProvider {
    public:
        virtual bool ShouldClose() const = 0;
        virtual void Close() = 0;
        virtual glm::i32vec2 GetFramebufferSize() const = 0;
        virtual glm::i32vec2 GetWindowSize() const = 0;
        virtual void SetFramebufferSize(uint width, uint height) = 0;
        virtual bool GetIsFullscreen() const = 0;
        virtual Handle<RenderCanvas> GetCanvas() const = 0;
        virtual float GetContentScale() const = 0;
        virtual const WindowParams& GetDesc() const = 0;
        virtual const void* GetNativeWindow() const = 0;
        virtual uint GetId() const = 0;
        virtual void SetCursor(ICursor* cursor) = 0;
        virtual void* GetWin32Window() = 0;

        virtual ~IWindow() = default;
    };

    class IDisplay {
    public:
        virtual IWindow* CreateWindow(
            const WindowParams& params = WindowParams()) = 0;
        virtual void DestroyWindow(IWindow* window) = 0;
        virtual GraphicsBackend GetRequestedBackend() const = 0;
        virtual ICursor* CreateStandardCursor(StandardCursor) = 0;
        virtual void DestroyCursor(ICursor*) = 0;

        virtual ~IDisplay() = default;
    };

    struct EditorCameraTag {
    };

    struct GizmoSelectTag {
    };

    enum class GizmoSelectionMode {
        TRANSLATION,
        ROTATION,
        SCALE
    };

    class IGizmo {
    public:
        virtual void SetMode(GizmoSelectionMode mode) = 0;
        virtual GizmoSelectionMode GetMode() const = 0;
    };

    std::unique_ptr<core::ISystem> CreateGLFWDisplay(
        core::ResourceInterface* resourceInterface,
        const RealtimeGraphicsParams& params = RealtimeGraphicsParams());

    std::unique_ptr<core::ISystem> CreateRenderer(
        IDisplay* displaySystem, 
        core::ResourceInterface& resources);

    std::unique_ptr<core::ISystem> CreateImGui(
        IDisplay* display,
        IRenderer* renderer, 
        IWindow* window);
    std::unique_ptr<core::ISystem> CreateImGui(
        IDisplay* display,
        IRenderer* renderer);

    std::unique_ptr<core::ISystem> CreateIm3d(
        IRenderer* renderer,
        core::IInputProvider* input,
        RenderCanvas* canvas);
    std::unique_ptr<core::ISystem> CreateIm3d(
        IRenderer* renderer,
        IWindow* window);
    std::unique_ptr<core::ISystem> CreateIm3d(
        IRenderer* renderer);

    std::unique_ptr<core::ISystem> CreateEditorSystem(
        IWindow* mainWindow,
        core::ISystem* imgui,
        core::ISystem* im3d,
        core::ISystem* gizmo
    );

    class IImGuiCallback {
    public:
        virtual core::delegate_handle_t Add(
            IWindow* window, 
            immedate_callback_t callback) = 0;
        virtual void Remove(core::delegate_handle_t handle) = 0;
    };

    class IIm3dSystem {
    public:
        virtual core::delegate_handle_t Add(
            RenderCanvas* canvas,
            core::IInputProvider* input,
            immedate_callback_t callback) = 0;
        virtual void Remove(core::delegate_handle_t handle) = 0;
        virtual core::delegate_handle_t AddNoDepth(
            RenderCanvas* canvas,
            core::IInputProvider* input,
            immedate_callback_t callback) = 0;

        inline core::delegate_handle_t Add(
            IWindow* window,
            immedate_callback_t callback) {
            return Add(window->GetCanvas(),
                window, std::move(callback));
        }

        inline core::delegate_handle_t AddNoDepth(
            IWindow* window,
            immedate_callback_t callback) {
            return AddNoDepth(window->GetCanvas(), 
                window, std::move(callback));
        }
    };
}