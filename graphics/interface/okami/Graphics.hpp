#pragma once

#include <okami/System.hpp>
#include <glm/vec2.hpp>

namespace okami::graphics {

    enum class GraphicsBackend {
        VULKAN,
        OPENGL,
        D3D11,
        D3D12
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
    inline std::unique_ptr<core::ISystem> CreateDisplay(
        const RealtimeGraphicsParams& params  = RealtimeGraphicsParams()) {
        return CreateGLFWDisplay(params);
    }

    std::unique_ptr<core::ISystem> CreateRenderer(
        core::ISystem* displaySystem, 
        core::ResourceInterface& resources);
}