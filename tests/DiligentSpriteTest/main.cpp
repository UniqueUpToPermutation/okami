#include <okami/Okami.hpp>
#include <okami/Graphics.hpp>
#include <okami/Geometry.hpp>
#include <okami/Transform.hpp>
#include <okami/GraphicsComponents.hpp>
#include <okami/Camera.hpp>

#include <iostream>
#include <random>

#include <marl/defer.h>

using namespace okami::core;
using namespace okami::graphics;

// Create all the objects on screen
struct SpriteAnimData {
    float mAngularVelocity;
    glm::vec2 mPositionBase;
    glm::vec2 mOscillatorVector;
    float mOscillatorVelocity;
    float mOscillatorX;
};

void SpriteUpdater(Frame& frame,
    UpdaterReads<>& reads, 
    UpdaterWrites<Transform>& writes,
    UpdaterWaits<>& waits,
    const Time& time) {

    writes.Write<Transform>([&]() {
        auto view = frame.Registry().view<Transform, SpriteAnimData>();

        for (auto e : view) {
            auto& transform = view.get<Transform>(e);
            auto& anim = view.get<SpriteAnimData>(e);

            anim.mOscillatorX += anim.mOscillatorVelocity * time.mTimeElapsed;
            
            auto position = anim.mPositionBase + std::cos(anim.mOscillatorX) * anim.mOscillatorVector;

            transform.mTranslation = glm::vec3(position.x, position.y, 0.0);
            transform.mRotation = transform.mRotation * glm::angleAxis(
                (float)(anim.mAngularVelocity * time.mTimeElapsed), glm::vec3(0.0f, 0.0f, 1.0f));
        }
    });
}

void TestBackend(GraphicsBackend backend) {
    RealtimeGraphicsParams gfxParams;
    gfxParams.mBackend = backend;

    WindowParams windowParams;

    switch (backend) {
        case GraphicsBackend::VULKAN:
            windowParams.mWindowTitle = "okami Diligent-Engine Sprite Test (Vulkan)";
            break;
        case GraphicsBackend::D3D11:
            windowParams.mWindowTitle = "okami Diligent-Engine Sprite Test (D3D11)";
            break;
        case GraphicsBackend::D3D12:
            windowParams.mWindowTitle = "okami Diligent-Engine Sprite Test (D3D12)";
            break;
    }

    ResourceManager resources;
    SystemCollection systems;
    systems.Add(CreateGLFWDisplay(&resources, gfxParams));
    auto display = systems.QueryInterface<IDisplay>();

    systems.Add(CreateRenderer(display, resources));
    auto renderer = systems.QueryInterface<IRenderer>();

    systems.Add(CreateUpdaterSystem(&SpriteUpdater));
    
    systems.Startup();
    {
        auto window = display->CreateWindow(windowParams);

        auto displayInterface = systems.QueryInterface<IDisplay>();
        auto vertexLayouts = systems.QueryInterface<IVertexLayoutProvider>();
        auto staticMeshLayout = vertexLayouts->GetVertexLayout<StaticMesh>();

        // Load a texture from disk
        auto spriteTexture = resources.Add(Texture("sprite.png"));

        Frame frame;
        resources.Add(&frame);

        // Create an orthographic camera for 2D rendering
        auto cameraEntity = frame.CreateEntity();
        auto& camera = frame.Emplace<Camera>(cameraEntity);
        camera.mType = Camera::Type::ORTHOGRAPHIC;
        camera.mNearPlane = -1.0f;
        camera.mFarPlane = 1.0f;
    
        // Geometry and texture are available to use after this is called.
        systems.SetFrame(frame);
        systems.LoadResources();

        // Randomly create a bunch of sprite objects
		std::default_random_engine generator;
		std::uniform_real_distribution<double> distribution1(-1.0, 1.0);
		std::uniform_real_distribution<double> distribution2(0.0, 1.0);

		constexpr uint obj_count = 350;
        constexpr float scale = 0.5;

        auto& texDesc = resources.Get<Texture>(spriteTexture).GetDesc();

        auto initialScreenSize = window->GetFramebufferSize();
        for (uint i = 0; i < obj_count; ++i) {
            SpriteAnimData anim;
			anim.mPositionBase.x = distribution1(generator) * initialScreenSize.x / 2.0;
			anim.mPositionBase.y = distribution1(generator) * initialScreenSize.y / 2.0;
			anim.mAngularVelocity = distribution1(generator) * 1.0;
			anim.mOscillatorVector.x = distribution1(generator) * 500.0;
			anim.mOscillatorVector.y = distribution1(generator) * 500.0;
			anim.mOscillatorVelocity = distribution1(generator) * 1.0;
			anim.mOscillatorX = distribution1(generator) * glm::pi<float>();

            Sprite sprite;
			sprite.mColor = glm::vec4(distribution2(generator), distribution2(generator), distribution2(generator), 1.0);
            sprite.mOrigin = glm::vec2(scale * texDesc.mWidth / 2.0, scale * texDesc.mHeight / 2.0);
            sprite.mTexture = spriteTexture;

            Transform transform;
            transform.mRotation = glm::angleAxis(
                (float)(distribution1(generator) * glm::pi<float>()), glm::vec3(0.0f, 0.0f, 1.0f));
            transform.mTranslation = glm::vec3(anim.mPositionBase.x, anim.mPositionBase.y, 0.0f);
            transform.mScale = glm::vec3(scale, scale, scale);

            auto entity = frame.CreateEntity();
            frame.Emplace<SpriteAnimData>(entity, anim);
            frame.Emplace<Sprite>(entity, sprite);
            frame.Emplace<Transform>(entity, transform);
		}

        RenderView rv;
        rv.bClear = true;
        rv.mCamera = cameraEntity;
        rv.mTargetId = window->GetCanvas()->GetResourceId();

        Clock clock;
        while (!window->ShouldClose()) {
            // Get the framebuffer size for the camera
            camera.mOrthoSize = window->GetFramebufferSize();
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