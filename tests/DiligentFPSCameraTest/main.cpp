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

    RealtimeGraphicsParams params;
    params.mDeviceType = backend;

    switch (backend) {
        case GraphicsBackend::VULKAN:
            params.mWindowTitle = "okami Diligent-Engine FPS Camera Test (Vulkan)";
            break;
        case GraphicsBackend::OPENGL:
            params.mWindowTitle = "okami Diligent-Engine FPS Camera Test (OpenGL)";
            break;
        case GraphicsBackend::D3D11:
            params.mWindowTitle = "okami Diligent-Engine FPS Camera Test (D3D11)";
            break;
        case GraphicsBackend::D3D12:
            params.mWindowTitle = "okami Diligent-Engine FPS Camera Test (D3D12)";
            break;
    }

    ResourceInterface resources;
    SystemCollection systems;
    auto display = systems.Add(CreateGLFWDisplay(params));
    auto renderer = systems.Add(CreateRenderer(display, resources));
    systems.Add(CreateFPSCameraSystem(display));

    systems.Startup();
    {
        auto displayInterface = systems.QueryInterface<IDisplay>();
        auto vertexLayouts = systems.QueryInterface<IVertexLayoutProvider>();
        auto staticMeshLayout = vertexLayouts->GetVertexLayout<StaticMesh>();

        // Create a geometry object from a built-in prefab
        auto geo = resources.Add(Geometry::Prefabs::MaterialBall(staticMeshLayout));
        // Load a texture from disk
        auto texture = resources.Load<Texture>("test.png");

        // Create a material for that texture
        BaseMaterial::Data materialData;
        materialData.mAlbedo = texture;
        auto material = resources.Add<BaseMaterial>(
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