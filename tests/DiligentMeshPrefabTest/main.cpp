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

    RealtimeGraphicsParams params;
    params.mDeviceType = backend;

    switch (backend) {
        case GraphicsBackend::VULKAN:
            params.mWindowTitle = "okami Diligent-Engine Mesh Prefab Test (Vulkan)";
            break;
        case GraphicsBackend::OPENGL:
            params.mWindowTitle = "okami Diligent-Engine Mesh Prefab Test (OpenGL)";
            break;
        case GraphicsBackend::D3D11:
            params.mWindowTitle = "okami Diligent-Engine Mesh Prefab Test (D3D11)";
            break;
        case GraphicsBackend::D3D12:
            params.mWindowTitle = "okami Diligent-Engine Mesh Prefab Test (D3D12)";
            break;
    }

    ResourceInterface resources;
    SystemCollection systems;
    auto display = systems.Add(CreateDisplay(params));
    auto renderer = systems.Add(CreateRenderer(display, resources));

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
    
        // Geometry and texture are available to use after this is called.
        systems.SetFrame(frame);
        systems.LoadResources();

        // Apply a transformation based on the bounding box to make sure that
        // the camera is a good distance away from the model
        auto aabb = geo->GetBoundingBox();
        glm::vec3 modelCenter = (aabb.mUpper + aabb.mLower) / 2.0f;
        float modelRadius = glm::length((aabb.mUpper - aabb.mLower) / 2.0f);

        auto& cameraTransform = frame.Emplace<Transform>(cameraEntity,
            Transform::LookAt(
                1.5f * modelRadius * glm::vec3(1.0f, 1.0f, 1.0f) + modelCenter,
                glm::vec3(0.0f, 0.0f, 0.0f) + modelCenter,
                glm::vec3(0.0f, 1.0f, 0.0f)
            ));
        
        Clock clock;
        while (!displayInterface->ShouldClose()) {
            auto time = clock.GetTime();
            // Rotate the mesh
            meshTransform.mRotation = 
                glm::angleAxis(time.mTotalTime, glm::dvec3(0.0, 1.0, 0.0));
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