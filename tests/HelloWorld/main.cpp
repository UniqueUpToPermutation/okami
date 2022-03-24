#include <okami/Okami.hpp>
#include <okami/ResourceManager.hpp>
#include <okami/Transform.hpp>

#include <marl/defer.h>

using namespace okami::core;

int main() {
    Meta::Register();

    marl::Scheduler scheduler(marl::Scheduler::Config::allCores());
    scheduler.bind();
    defer(scheduler.unbind());

    ResourceManager resources;

    SystemCollection systems;

    systems.Startup();
    {
        Frame frame;
        auto entity = frame.CreateEntity(frame.GetRoot());
        frame.Emplace<Transform>(entity, Transform());
        resources.Add(&frame);

        systems.SetFrame(frame);
        systems.LoadResources();
    
        systems.Fork(Time{0.0, 0.0});
        systems.Join();

        resources.Free(&frame);
    }
    systems.Shutdown();
}