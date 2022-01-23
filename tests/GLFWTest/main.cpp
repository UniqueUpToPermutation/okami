#include <okami/Okami.hpp>
#include <okami/Graphics.hpp>

#include <iostream>
#include <marl/defer.h>

using namespace okami::core;
using namespace okami::graphics;

int main() {
    Meta::Register();

    ResourceInterface resources;

    marl::Scheduler scheduler(marl::Scheduler::Config::allCores());
    scheduler.bind();
    defer(scheduler.unbind());

    SystemCollection systems;
    systems.Add(CreateGLFWDisplay(&resources));

    auto display = systems.QueryInterface<IDisplay>();
    auto window = display->CreateWindow();

    systems.Startup();
    {
        Frame frame;
        systems.SetFrame(frame);
        systems.LoadResources();
        
        while (!window->ShouldClose()) {
            systems.Fork(Time{0.0, 0.0});
            systems.Join();
        }
    }
    systems.Shutdown();
}