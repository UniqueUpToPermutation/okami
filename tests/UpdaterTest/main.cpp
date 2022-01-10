#include <okami/Okami.hpp>
#include <okami/Transform.hpp>
#include <okami/System.hpp>
#include <okami/Camera.hpp>

#include <marl/defer.h>
#include <iostream>

using namespace okami::core;

std::atomic<int> gTransformReadsRemaining = 0;
std::atomic<int> gCameraReadsRemaining = 0;
std::atomic<int> gTransformWritesRemaining = 0;
std::atomic<int> gCameraWritesRemaining = 0;

#define TEST_ASSERT(x) \
    if (!x) { \
        std::cerr << \
            "LINE " << __LINE__  << ": Test Failed: " << #x << std::endl; \
        throw std::runtime_error("Test Failed!"); \
    }


void Updater1(Frame& frame, 
    UpdaterReads<Transform, Camera>& reads,
    UpdaterWrites<Transform, Camera>& writes,
    UpdaterWaits<Transform, Camera>& waits,
    const Time& time) {
    reads.Read<Camera>([]() {
        --gCameraReadsRemaining;
    });

    reads.Read<Transform>([]() {
        --gTransformReadsRemaining;
    });

    writes.Write<Camera>([]() { 
        TEST_ASSERT(gCameraReadsRemaining == 0);
        --gCameraWritesRemaining;
    });

    writes.Write<Transform>([]() {
        TEST_ASSERT(gTransformReadsRemaining == 0);
        --gTransformWritesRemaining;
    });

    waits.Wait<Transform>();
    TEST_ASSERT(gTransformWritesRemaining == 0);

    waits.Wait<Camera>();
    TEST_ASSERT(gCameraWritesRemaining == 0);
}

void Updater2(Frame& frame, 
    UpdaterReads<Transform, Camera>& reads,
    UpdaterWrites<Transform, Camera>& writes,
    UpdaterWaits<Transform, Camera>& waits,
    const Time& time) {
    reads.Read<MultiRead<Transform, Camera>>([]() {
        --gTransformReadsRemaining;
        --gCameraReadsRemaining;
    });

    writes.Write<Transform>([]() {
        TEST_ASSERT(gTransformReadsRemaining == 0);
        --gTransformWritesRemaining;
    });

    writes.Write<Camera>([]() {
        TEST_ASSERT(gCameraReadsRemaining == 0);
        --gCameraWritesRemaining;
    });

    waits.Wait<Camera>();
    TEST_ASSERT(gCameraWritesRemaining == 0);

    waits.Wait<Transform>();
    TEST_ASSERT(gTransformWritesRemaining == 0);
}

int main() {
    Meta::Register();

    marl::Scheduler scheduler(marl::Scheduler::Config::allCores());
    scheduler.bind();
    defer(scheduler.unbind());

    ResourceInterface resources;

    SystemCollection systems;
    systems.Add(CreateUpdaterSystem(&Updater1));
    systems.Add(CreateUpdaterSystem(&Updater2));

    systems.Startup();
    {
        Frame frame;
        auto entity = frame.CreateEntity(frame.GetRoot());
        frame.Emplace<Transform>(entity);

        auto entity2 = frame.CreateEntity(frame.GetRoot());
        frame.Emplace<Transform>(entity2);

        systems.SetFrame(frame);
        systems.LoadResources();
    
        for (size_t i = 0; i < 100000; ++i) {
            gTransformReadsRemaining = 2;
            gCameraReadsRemaining = 2;
            gTransformWritesRemaining = 2;
            gCameraWritesRemaining = 2;
            systems.Fork(Time{0.0, 0.0});
            systems.Join();
        }
    }
    systems.Shutdown();
}