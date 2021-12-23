#include <okami/Okami.hpp>
#include <okami/Graphics.hpp>

#include <iostream>
#include <marl/defer.h>

using namespace okami::core;
using namespace okami::graphics;

int main() {
    Meta::Register();

    marl::Scheduler scheduler(marl::Scheduler::Config::allCores());
    scheduler.bind();
    defer(scheduler.unbind());

    SystemCollection systems;
    systems.Add(CreateDisplay());

    systems.Startup();
    {
        auto window = systems.QueryInterface<IDisplay>();

        Frame frame;
        systems.SetFrame(frame);
        systems.LoadResources();
        
        while (!window->ShouldClose()) {
            systems.BeginExecute(Time{0.0, 0.0});
            systems.EndExecute();
        }
    }
    systems.Shutdown();
}