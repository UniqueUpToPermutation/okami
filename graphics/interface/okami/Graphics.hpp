#pragma once

#include <okami/System.hpp>
#include <okami/Event.hpp>
#include <glm/vec2.hpp>

namespace okami::graphics {

    typedef std::function<void()> immedate_callback_t;

    class IImmediateModeCallback {
    public:
        virtual core::delegate_handle_t Add(immedate_callback_t callback) = 0;
        virtual void Remove(core::delegate_handle_t handle) = 0;
        virtual marl::WaitGroup& GetUpdateWaitGroup() = 0;
    };

    class IImGuiCallback : public IImmediateModeCallback {
    };

    class IIm3dCallback : public IImmediateModeCallback {
    };

    enum class GraphicsBackend {
        VULKAN,
        OPENGL,
        D3D11,
        D3D12
    };

    class IGraphicsObject {
    public:
        virtual ~IGraphicsObject() = default;
    };

    class IRenderer {
    public:
        virtual void AddModule(std::unique_ptr<IGraphicsObject>&&) = 0;
        virtual void AddOverlay(IGraphicsObject* object) = 0;
        virtual void RemoveOverlay(IGraphicsObject* object) = 0;
        virtual glm::i32vec2 GetRenderArea() const = 0;
        virtual void Wait() = 0;
    };

    class IDisplay {
    public:
        virtual bool ShouldClose() const = 0;
        virtual glm::i32vec2 GetFramebufferSize() const = 0;
        virtual bool GetIsFullscreen() const = 0;
        virtual GraphicsBackend GetRequestedBackend() const = 0;
    };

    constexpr uint DEFAULT_WINDOW_WIDTH = 1920;
    constexpr uint DEFAULT_WINDOW_HEIGHT = 1080;

    constexpr GraphicsBackend DEFAULT_BACKEND = GraphicsBackend::VULKAN;

    struct RealtimeGraphicsParams {
		std::string mWindowTitle 	= "okami";
		uint mBackbufferWidth 	    = DEFAULT_WINDOW_WIDTH;
		uint mBackbufferHeight 		= DEFAULT_WINDOW_HEIGHT;
		bool bFullscreen 			= false;
		bool bShowOnCreation		= true;
		GraphicsBackend mDeviceType = DEFAULT_BACKEND;
	};

    std::unique_ptr<core::ISystem> CreateGLFWDisplay(
        const RealtimeGraphicsParams& params = RealtimeGraphicsParams());
    std::unique_ptr<core::ISystem> CreateDisplay(
        const RealtimeGraphicsParams& params = RealtimeGraphicsParams());
    std::unique_ptr<core::ISystem> CreateRenderer(
        core::ISystem* displaySystem, 
        core::ResourceInterface& resources);
    std::unique_ptr<core::ISystem> CreateImGui(
        IRenderer* renderer,
        core::ISystem* input);
    std::unique_ptr<core::ISystem> CreateIm3d(
        IRenderer* renderer);
}