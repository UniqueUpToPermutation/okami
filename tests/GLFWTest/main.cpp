#include <okami/Okami.hpp>
#include <okami/Graphics.hpp>

#include <iostream>
#include <marl/defer.h>

using namespace okami;
using namespace okami::core;
using namespace okami::graphics;

int main() {
    Meta::Register();

    ResourceManager resources;

    marl::Scheduler scheduler(marl::Scheduler::Config::allCores());
    scheduler.bind();
    defer(scheduler.unbind());

    // Create the GLFW Display system
    SystemCollection systems;
    systems.Add(CreateGLFWDisplay(&resources));

    systems.Startup();
    {
        // Query the display.
        // The display is an interface similar to GLFW, but abstracted
        // so that you don't actually have to make direct calls to GLFW.
        auto display = systems.QueryInterface<IDisplay>();

        // Create two windows.
        WindowParams windowParams;
        windowParams.mWindowTitle = "GLFW Test (Window 1)";
        windowParams.bIsPrimary = true;
        auto window1 = display->CreateWindow(windowParams);

        windowParams.mWindowTitle = "GLFW Test (Window 2)";
        windowParams.bIsPrimary = false;
        auto window2 = display->CreateWindow(windowParams);

        // Create a frame and load resources.
        Frame frame;

        resources.Add(&frame);

        systems.SetFrame(frame);
        systems.LoadResources();
        
        // Run the game loop while the first window is open.
        while (!window1->ShouldClose()) {

            // Close the second window if requested.
            if (window2 && window2->ShouldClose()) {
                window2->Close();

                // After the window has been closed,
                // the pointer is no longer valid!
                window2 = nullptr;
            }

            systems.Fork(Time{0.0, 0.0});
            systems.Join();
        }

        resources.Free(&frame);
    }
    systems.Shutdown();
}