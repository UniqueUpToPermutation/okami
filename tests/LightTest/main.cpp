#include <okami/Okami.hpp>
#include <okami/Graphics.hpp>
#include <okami/Geometry.hpp>
#include <okami/Transform.hpp>
#include <okami/GraphicsComponents.hpp>
#include <okami/Camera.hpp>

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
            windowParams.mWindowTitle = "okami Lighting Test (Vulkan)";
            break;
        case GraphicsBackend::D3D11:
            windowParams.mWindowTitle = "okami Lighting Prefab Test (D3D11)";
            break;
        case GraphicsBackend::D3D12:
            windowParams.mWindowTitle = "okami Lighting Prefab Test (D3D12)";
            break;
    }

    ResourceManager resources;
    SystemCollection systems;
    systems.Add(CreateGLFWDisplay(&resources, gfxParams));
    auto display = systems.QueryInterface<IDisplay>(); 

    systems.Add(CreateRenderer(display, resources));
    auto renderer = systems.QueryInterface<IRenderer>(); 

    systems.Startup();
    {
        auto window = display->CreateWindow(windowParams);

        auto vertexLayouts = systems.QueryInterface<IVertexLayoutProvider>();
        auto staticMeshLayout = vertexLayouts->GetVertexLayout<StaticMesh>();

        Frame frame;
        resources.Add(&frame);

        // Create a geometry object from a built-in prefab
        auto geo = resources.Add(
            Geometry::Prefabs::MaterialBall(staticMeshLayout), frame);
        // Load a texture from disk
        auto texture = resources.Add(Texture("test.png"), frame);

        // Create a material for that texture
        PhongSurface materialSurface;
        materialSurface.mAlbedo = texture;
        materialSurface.mSpecularPower = 12.0;
        materialSurface.mSpecularFactor = 1.0;
        auto material = resources.Add<Material<StaticMesh>>(
            Material<StaticMesh>(materialSurface));

        // Create a frame with the static mesh at the origin
        auto staticMeshEntity = frame.CreateEntity();
        auto& meshTransform = frame.Emplace<Transform>(staticMeshEntity);
        frame.Emplace<StaticMesh>(staticMeshEntity, StaticMesh{geo, material});

        // Create a directional light
        auto lightEntity = frame.CreateEntity();
        auto& lightTransform = frame.Emplace<Transform>(lightEntity);
        auto& lightAttribs = frame.Emplace<DirectionalLight>(lightEntity);

        // Create a camera
        auto cameraEntity = frame.CreateEntity();
        auto& camera = frame.Emplace<Camera>(cameraEntity);
    
        // Geometry and texture are available to use after this is called.
        systems.SetFrame(frame);
        systems.LoadResources();

        // Apply a transformation based on the bounding box to make sure that
        // the camera is a good distance away from the model
        auto aabb = resources.Get<Geometry>(geo).GetBoundingBox();
        glm::vec3 modelCenter = (aabb.mUpper + aabb.mLower) / 2.0f;
        float modelRadius = glm::length((aabb.mUpper - aabb.mLower) / 2.0f);

        auto& cameraTransform = frame.Emplace<Transform>(cameraEntity,
            Transform::LookAt(
                1.5f * modelRadius * glm::vec3(1.0f, 1.0f, 1.0f) + modelCenter,
                glm::vec3(0.0f, 0.0f, 0.0f) + modelCenter,
                glm::vec3(0.0f, 1.0f, 0.0f)
            ));

        RenderView rv;
        rv.bClear = true;
        rv.mCamera = cameraEntity;
        rv.mTargetId = window->GetCanvas()->GetResourceId();
        
        Clock clock;
        while (!window->ShouldClose()) {
            auto time = clock.GetTime();

            // Rotate the light
            lightTransform.mRotation = 
                glm::angleAxis(-time.mTotalTime * 0.37, glm::dvec3(0.0, 1.0, 0.0)) *
                glm::angleAxis(0.75, glm::dvec3(1.0, 0.0, 0.0));

            // Rotate the mesh and set render view
            meshTransform.mRotation = 
                glm::angleAxis(time.mTotalTime, glm::dvec3(0.0, 1.0, 0.0));
            renderer->SetRenderView(rv);

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