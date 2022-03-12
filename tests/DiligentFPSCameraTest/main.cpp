#include <okami/Okami.hpp>
#include <okami/Graphics.hpp>
#include <okami/Geometry.hpp>
#include <okami/Transform.hpp>
#include <okami/GraphicsComponents.hpp>
#include <okami/Camera.hpp>

#include <okami/diligent/FirstPersonCamera.hpp>

#include <iostream>
#include <marl/defer.h>

using namespace okami::core;
using namespace okami::graphics;

void TestBackend(GraphicsBackend backend) {

    RealtimeGraphicsParams gfxParams;
    gfxParams.mBackend = backend;

    WindowParams windowParams;

    switch (backend) {
        case GraphicsBackend::VULKAN:
            windowParams.mWindowTitle = "okami Diligent-Engine FPS Camera Test (Vulkan)";
            break;
        case GraphicsBackend::D3D11:
            windowParams.mWindowTitle = "okami Diligent-Engine FPS Camera Test (D3D11)";
            break;
        case GraphicsBackend::D3D12:
            windowParams.mWindowTitle = "okami Diligent-Engine FPS Camera Test (D3D12)";
            break;
    }

    ResourceInterface resources;
    SystemCollection systems;
    systems.Add(CreateGLFWDisplay(&resources, gfxParams));
    auto display = systems.QueryInterface<IDisplay>();

    systems.Add(CreateRenderer(display, resources));
    auto renderer = systems.QueryInterface<IRenderer>();

    systems.Startup();
    {
        auto window = display->CreateWindow(windowParams);
        auto cameraSystem = CreateFPSCameraSystem(window);
        cameraSystem->Startup();
        systems.Add(std::move(cameraSystem));

        auto displayInterface = systems.QueryInterface<IDisplay>();
        auto vertexLayouts = systems.QueryInterface<IVertexLayoutProvider>();
        auto staticMeshLayout = vertexLayouts->GetVertexLayout<StaticMesh>();

        // Create a geometry object from a built-in prefab
        auto geo = resources.Add(Geometry::Prefabs::MaterialBall(staticMeshLayout));
        // Load a texture from disk
        auto texture = resources.Load<Texture>("test.png");

        // Create a material for that texture
        StaticMeshMaterial::Data materialData;
        materialData.mAlbedo = texture;
        auto material = resources.Add<StaticMeshMaterial>(
            std::move(materialData));

        // Create a frame with the static mesh at the origin
        Frame frame;
        auto staticMeshEntity = frame.CreateEntity();
        auto& meshTransform = frame.Emplace<Transform>(staticMeshEntity);
        frame.Emplace<StaticMesh>(staticMeshEntity, StaticMesh{geo, material});

        // Create a camera
        auto cameraEntity = frame.CreateEntity();
        auto& camera = frame.Emplace<Camera>(cameraEntity);

        frame.Emplace<Transform>(cameraEntity,
            Transform().SetTranslate(0.0f, 0.0f, -4.0f));
        frame.Emplace<FirstPersonController>(cameraEntity);
    
        // Geometry and texture are available to use after this is called.
        systems.SetFrame(frame);
        systems.LoadResources();

        RenderView rv;
        rv.bClear = true;
        rv.mCamera = cameraEntity;
        rv.mTarget = window->GetCanvas();

        Clock clock;
        while (!window->ShouldClose()) {
            auto time = clock.GetTime();

            renderer->SetRenderView(rv);
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

#if D3D11_SUPPORTED
    TestBackend(GraphicsBackend::D3D11);
#endif

#if D3D12_SUPPORTED
    TestBackend(GraphicsBackend::D3D12);
#endif
}