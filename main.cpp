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

// TODO: Rationalize which headers are needed here - right now I'm including everything to make sure it all compiles...

#include "global.h"
#include "canvas.h"
#include "interfaces.h"
#include "serialization.h"
#include "socketchannel.h"
#include "socketcontroller.h"
#include "ledeffectbase.h"
#include "ledfeature.h"
#include "utilities.h"
#include "server.h"
#include "effectsmanager.h"
#include "greenfilleffect.h"
#include "colorwaveeffect.h"    

using namespace std;

// Global Objects
WebServer webServer;

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
// object is configured with a specific LED matrix and effect.  This function is
// called once at the beginning of the program to set up the LED matrix and effects.

std::vector<std::shared_ptr<ICanvas>> LoadCanvases()
{
    std::vector<std::shared_ptr<ICanvas>> canvases;

    // Define a Canvas with dimensions matching the sign
    auto canvas = make_shared<Canvas>(512, 32);

    // Define a GreenFillEffect
    auto colorWaveEffect = make_shared<ColorWaveEffect>("Color Wave");

    // Add LEDFeature for a specific client (example IP: "192.168.1.100")
    auto feature1 = make_shared<LEDFeature>
    (
        canvas,             // Canvas to get pixels from
        "192.168.8.176",    // Hostname
        "Workbench Matrix", // Friendly Name
        512, 32,            // Width, Height
        0, 0,               // Offset X, Offset Y
        false,              // Reversed
        0,                  // Channel
        false,              // Red-Green Swap
        1                   // Batch Size
    );
    // Add features to the canvas
    canvas->AddFeature(feature1);

    // Add the effect to the EffectsManager
    canvas->Effects().AddEffect(colorWaveEffect);
    canvas->Effects().SetCurrentEffect(0, *canvas);

    // Add the canvas to the list of canvases
    canvases.push_back(canvas);

    return canvases;
}

// Main program entry point. Runs the webServer and starts up the LED processing.
// When SIGINT is received, exits gracefully.

int main(int, char *[])
{
    std::vector<std::shared_ptr<ICanvas>> allCanvases;    
    SocketController socketController;

    // Register signal handler for SIGINT
    signal(SIGINT, handle_signal);

    // Start the web server
    pthread_t serverThread = webServer.Start();
    if (!serverThread)
    {
        cerr << "Failed to start the server thread\n";
        return 1;
    }

    cout << "Started server, waiting..." << endl;

    // Load the canvases and features
    allCanvases = LoadCanvases();

    // Add a SocketChannel for each feature and start the drawing of effects
    socketController.AddChannelsForCanvases(allCanvases);
    socketController.StartAll();

    // Main application loop.  EffectManagers draw frames to the canvas queue, and those frames
    // are then compressed and sent to the LED matrix via the SocketController threads.

    while (running)
        this_thread::sleep_for(chrono::milliseconds(100));

    // Stop all channels on exit
    socketController.StopAll();

    cout << "Stopping server..." << endl;
    webServer.Stop();
    return 0;
}
