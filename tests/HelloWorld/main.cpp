#include <okami/Okami.hpp>
#include <okami/Transform.hpp>

#include <marl/defer.h>

using namespace okami::core;

int main() {
    Meta::Register();

    marl::Scheduler scheduler(marl::Scheduler::Config::allCores());
    scheduler.bind();
    defer(scheduler.unbind());

    ResourceInterface resources;

    SystemCollection systems;
    systems.Add(FrameSystemFactory(resources));

    systems.Startup();
    {
        Frame frame;
        auto entity = frame.CreateEntity(frame.GetRoot());
        frame.Emplace<Transform>(entity, Transform());

        systems.SetFrame(frame);
        systems.LoadResources();
    
        systems.Fork(Time{0.0, 0.0});
        systems.Join();
    }
    systems.Shutdown();
}