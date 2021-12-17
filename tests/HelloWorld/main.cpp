#include <okami/Okami.hpp>
#include <okami/Transform.hpp>

#include <marl/defer.h>

using namespace okami::core;

int main() {
    marl::Scheduler scheduler(marl::Scheduler::Config::allCores());
    scheduler.bind();
    defer(scheduler.unbind());

    ResourceInterface resources;

    SystemCollection systems;
    systems.Add(FrameSystemFactory(resources));

    Frame frame;
    auto entity = frame.CreateEntity(frame.GetRoot());
    frame.Emplace<Transform>(entity, Transform());

    systems.Startup();
    systems.LoadResources(&frame);
    systems.BeginExecute(&frame, Time{0.0, 0.0});
    systems.EndExecute();
    systems.Shutdown();
}