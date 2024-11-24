// Main.cpp
// 
// This file is the main entry point for the NDSCPP LED Matrix Server application.
// It creates a Canvas, adds a GreenFillEffect to it, and then enters a loop where it
// renders the effect to the canvas, compresses the data, and sends it to the LED
// matrix via a SocketChannel.  The program will continue to run until it receives
// a SIGINT signal (Ctrl-C).

#include <csignal>
#include <iostream>
#include <atomic>
#include <chrono>
#include <thread>

#include "global.h"
#include "canvas.h"
#include "interfaces.h"
#include "socketchannel.h"
#include "ledfeature.h"
#include "webserver.h"
#include "effectsmanager.h"
#include "colorwaveeffect.h"    
#include "starfield.h"

using namespace std;

// Atomic flag to indicate whether the program should continue running.
// Will be set to false when SIGINT is received.
atomic<bool> running(true);

void handle_signal(int signal)
{
    if (signal == SIGINT)
    {
        running = false;
        cerr << "Received SIGINT, exiting...\n" << flush;
    }
    else if (signal == SIGPIPE)
    {
        cerr << "Received SIGPIPE, ignoring...\n" << flush;
    }
}

// LoadCanvases
//
// Creates and returns a vector of shared pointers to Canvas objects.  Each Canvas
// object is configured with one or more LEDFeatures and effects.  This function is
// called once at the beginning of the program to set up the LED matrix and effects.

vector<unique_ptr<ICanvas>> LoadCanvases()
{
    vector<unique_ptr<ICanvas>> canvases;

    // Define a Canvas
    auto canvas = make_unique<Canvas>(512, 32, 30);

    // Add LEDFeature
    auto feature1 = make_shared<LEDFeature>(
        canvas.get(),         // Canvas pointer
        "192.168.8.176",      // Hostname
        "Workbench Matrix",   // Friendly Name
        49152,                // Port
        512, 32,              // Width, Height
        0, 0,                 // Offset X, Offset Y
        false,                // Reversed
        0,                    // Channel
        false                 // Red-Green Swap
    );
    canvas->AddFeature(std::move(feature1));

    // Add effect to EffectsManager
    canvas->Effects().AddEffect(make_unique<StarfieldEffect>("Starfield", 100));
    canvas->Effects().SetCurrentEffect(0, *canvas);

    canvases.push_back(std::move(canvas));
    return canvases;
}


// Main program entry point. Runs the webServer and starts up the LED processing.
// When SIGINT is received, exits gracefully.

int main(int, char *[])
{
    // Register signal handler for SIGINT
    signal(SIGINT, handle_signal);

    cout << "Loading canvases..." << endl;

    // Load the canvases and features

    vector<unique_ptr<ICanvas>> allCanvases = LoadCanvases();

    cout << "Connecting to clients..." << endl;

    // Connect and start the sockets to the clients

    for (const auto &canvas : allCanvases)
        for (const auto &feature : canvas->Features())
            feature->Socket().Start();
            
    // Start rendering effects

    for (const auto &canvas : allCanvases)
         canvas->Effects().Start(*canvas);

    cout << "Starting the Web Server..." << endl;

    // Start the web server
    
    WebServer webServer(allCanvases);
    pthread_t serverThread = webServer.Start();
    if (!serverThread)
    {
        cerr << "Failed to start the server thread\n";
        return 1;
    }

    cout << "[Entered Running State]" << endl;

    // Main application loop.  EffectManagers draw frames to the canvas queue, and those frames
    // are then compressed and sent to the LED matrix via the SocketController threads.

    while (running)
        this_thread::sleep_for(milliseconds(100));

    cout << "Stopping web server..." << endl;
    webServer.Stop();
    
    // Shut down rendering and communications

    for (const auto &canvas : allCanvases)
         canvas->Effects().Stop();

    for (const auto &canvas : allCanvases)
        for (const auto &feature : canvas->Features())
            feature->Socket().Stop();

    return 0;
}
