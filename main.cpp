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
#include "socketchannel.h"
#include "socketcontroller.h"
#include "ledeffectbase.h"
#include "ledfeature.h"
#include "utilities.h"
#include "server.h"
#include "effectsmanager.h"
#include "effects/greeenfilleffect.h"

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

SocketController socketController;
EffectsManager effectsManager;

// Main program entry point. Runs the webServer and starts up the LED processing.
// When SIGINT is received, exits gracefully.

int main(int, char *[])
{
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

    try
    {
        // Define a Canvas with dimensions matching the sign
        auto canvas = make_shared<Canvas>(512, 32);

        // Define a GreenFillEffect
        auto greenEffect = make_shared<GreenFillEffect>("Green Fill");

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

        // Add features to the SocketController
        socketController.AddChannel(
            feature1->HostName(),
            feature1->FriendlyName()
        );

        // Add the effect to the EffectsManager
        effectsManager.AddEffect(greenEffect);
        effectsManager.SetCurrentEffect(0, *canvas);

        // Start the SocketController
        socketController.StartAll();

        // Main application loop.  Draws frames, compresses them, queues them up for transmit
        
        while (running)
        {
            // Render the current effect to the canvas
            effectsManager.UpdateCurrentEffect(*canvas, 16ms); // Assume ~60 FPS (delta time = 1/60)

            // Send the data to each feature's SocketChannel
            for (const auto &feature : canvas->Features())
            {
                auto frame = feature->GetDataFrame();

                auto channel = socketController.FindChannelByHost(feature->HostName());
                auto compressedFrame = channel->CompressFrame(frame);

                if (channel)
                    channel->EnqueueFrame(compressedFrame);
            }

            // Wait to simulate ~60 FPS
            this_thread::sleep_for(chrono::milliseconds(16));
        }

        // Stop all channels on exit
        socketController.StopAll();
    }
    catch (const exception &e)
    {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    cout << "Stopping server..." << endl;
    webServer.Stop();
    return 0;
}
