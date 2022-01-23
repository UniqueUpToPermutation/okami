#include <okami/Okami.hpp>
#include <okami/Graphics.hpp>
#include <okami/Geometry.hpp>
#include <okami/Transform.hpp>
#include <okami/GraphicsComponents.hpp>
#include <okami/Camera.hpp>

#include <iostream>
#include <random>

#include <marl/defer.h>
#include <im3d.h>

using namespace okami::core;
using namespace okami::graphics;

void TestBackend(GraphicsBackend backend) {

    RealtimeGraphicsParams params;
    params.mDeviceType = backend;

    switch (backend) {
        case GraphicsBackend::VULKAN:
            params.mWindowTitle = "okami Diligent-Engine Im3d Test (Vulkan)";
            break;
        case GraphicsBackend::OPENGL:
            params.mWindowTitle = "okami Diligent-Engine Im3d Test (OpenGL)";
            break;
        case GraphicsBackend::D3D11:
            params.mWindowTitle = "okami Diligent-Engine Im3d Test (D3D11)";
            break;
        case GraphicsBackend::D3D12:
            params.mWindowTitle = "okami Diligent-Engine Im3d Test (D3D12)";
            break;
    }

    ResourceInterface resources;
    SystemCollection systems;
    auto display = systems.Add(CreateGLFWDisplay(params));
    auto renderer = systems.Add(CreateRenderer(display, resources));
    auto displayInterface = systems.QueryInterface<IDisplay>();

    systems.Add(CreateIm3d(renderer));

    auto im3dInterface = systems.QueryInterface<IIm3dSystem>();
    im3dInterface->Add([]() {
        Im3d::BeginTriangles();
        Im3d::Vertex(-0.75f, -0.75f, 0.0f, Im3d::Color_Blue);
        Im3d::Vertex(-0.5f, -0.25f, 0.0f, Im3d::Color_Green);
        Im3d::Vertex(-0.25f, -0.75f, 0.0f, Im3d::Color_Red);
        Im3d::End();

        Im3d::BeginLineLoop();
        Im3d::Vertex(-0.75f, 0.25f, 0.0f, 4.0f, Im3d::Color_Blue);
        Im3d::Vertex(-0.5f, 0.75f, 0.0f, 4.0f, Im3d::Color_Green);
        Im3d::Vertex(-0.25f, 0.25f, 0.0f, 4.0f, Im3d::Color_Red);
        Im3d::End();

        Im3d::DrawPoint(Im3d::Vec3(0.5f, 0.5f, 0.0f), 
            50.0f, Im3d::Color_Black);

        Im3d::DrawCircleFilled(Im3d::Vec3(0.5f, -0.5f, 0.0f), 
            Im3d::Vec3(0.0f, 0.0f, -1.0f), 0.25f);
    });

    systems.Startup();
    {
        Frame frame;

        // Geometry and texture are available to use after this is called.
        systems.SetFrame(frame);
        systems.LoadResources();

        Clock clock;
        while (!displayInterface->ShouldClose()) {
            auto time = clock.GetTime();
            systems.Fork(time);
            systems.Join();
        }
    }
    systems.Shutdown();
}

int main() {
    Meta::Register();

    marl::Scheduler scheduler(marl::Scheduler::Config::allCores());
    scheduler.bind();
    defer(scheduler.unbind());

#if VULKAN_SUPPORTED && !PLATFORM_MACOS
    TestBackend(GraphicsBackend::VULKAN);
#endif

#if GL_SUPPORTED
    TestBackend(GraphicsBackend::OPENGL);
#endif


#if D3D11_SUPPORTED
    TestBackend(GraphicsBackend::D3D11);
#endif

#if D3D12_SUPPORTED
    TestBackend(GraphicsBackend::D3D12);
#endif
}