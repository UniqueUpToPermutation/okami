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
    RealtimeGraphicsParams gfxParams;
    gfxParams.mBackend = backend;

    WindowParams wndParams;

    switch (backend) {
        case GraphicsBackend::VULKAN:
            wndParams.mWindowTitle = "okami Diligent-Engine Im3d Test (Vulkan)";
            break;
        case GraphicsBackend::D3D11:
            wndParams.mWindowTitle = "okami Diligent-Engine Im3d Test (D3D11)";
            break;
        case GraphicsBackend::D3D12:
            wndParams.mWindowTitle = "okami Diligent-Engine Im3d Test (D3D12)";
            break;
    }

    ResourceManager resources;
    SystemCollection systems;
    systems.Add(CreateGLFWDisplay(&resources, gfxParams));
    auto display = systems.QueryInterface<IDisplay>();
    
    systems.Add(CreateRenderer(display, resources));
    auto renderer = systems.QueryInterface<IRenderer>();

    systems.Add(CreateIm3d(renderer));
    auto im3dInterface = systems.QueryInterface<IIm3dSystem>();

    systems.Startup();
    {
        auto window = display->CreateWindow(wndParams);

        im3dInterface->Add(window, []() {
            Im3d::BeginTriangles();
            Im3d::Vertex(-0.75f, -0.75f, 0.0f, Im3d::Color_Blue);
            Im3d::Vertex(-0.5f, -0.25f, 0.0f, Im3d::Color_Green);
            Im3d::Vertex(-0.25f, -0.75f, 0.0f, Im3d::Color_Red);
            Im3d::End();

            // Algorithm for lines isn't perfect, 
            // lines at depth 0.0 will be culled currently.
            Im3d::BeginLineLoop();
            Im3d::Vertex(-0.75f, 0.25f, 0.1f, 4.0f, Im3d::Color_Blue);
            Im3d::Vertex(-0.5f, 0.75f, 0.1f, 4.0f, Im3d::Color_Green);
            Im3d::Vertex(-0.25f, 0.25f, 0.1f, 4.0f, Im3d::Color_Red);
            Im3d::End();

            Im3d::DrawPoint(Im3d::Vec3(0.5f, 0.5f, 0.0f), 
                50.0f, Im3d::Color_Black);

            Im3d::DrawCircleFilled(Im3d::Vec3(0.5f, -0.5f, 0.0f), 
                Im3d::Vec3(0.0f, 0.0f, -1.0f), 0.25f);
        });

        Frame frame;
        resources.Add(&frame);

        systems.SetFrame(frame);
        systems.LoadResources();

        RenderView rv;
        rv.bClear = true;
        rv.mCamera = entt::null;
        rv.mTargetId = window->GetCanvas()->GetResourceId();

        Clock clock;
        while (!window->ShouldClose()) {
            renderer->SetRenderView(rv);
            
            auto time = clock.GetTime();
            systems.Fork(time);
            systems.Join();
        }

        resources.Free(&frame);
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

#if D3D11_SUPPORTED
    TestBackend(GraphicsBackend::D3D11);
#endif

#if D3D12_SUPPORTED
    TestBackend(GraphicsBackend::D3D12);
#endif
}