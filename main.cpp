#include <csignal>
#include <iostream>
#include <atomic>
#include <chrono>
#include <thread>
#include "global.h"
#include "canvas.h"
#include "canvascontroller.h"
#include "clientchannel.h"
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
}

SocketController socketController;
EffectsManager effectsManager;

// Main program entry point. Runs the webServer and updates the known symbols list every hour.
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
        // Define a Canvas with dimensions 16x16
        auto canvas = make_shared<Canvas>(512, 32);

        // Define a GreenFillEffect
        auto greenEffect = make_shared<GreenFillEffect>("Green Fill");

        // Add LEDFeature for a specific client (example IP: "192.168.1.100")
        auto feature1 = make_shared<LEDFeature>
        (
            canvas,             // Canvas to get pixels from
            "192.168.8.176",    // Hostname
            "Workbench Matrix", // Friendly Name
            8, 8,               // Width, Height
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
            feature1->FriendlyName(),
            feature1->Width(),
            feature1->Height(),
            feature1->OffsetX(),
            feature1->Channel(),
            feature1->RedGreenSwap()
        );

        // Add the effect to the EffectsManager
        effectsManager.AddEffect(greenEffect);
        effectsManager.SetCurrentEffect(0, *canvas);

        // Start the SocketController
        socketController.StartAll();

        // Main application loop
        while (running)
        {
            // Get the current time
            auto startTime = chrono::system_clock::now();

            // Render the current effect to the canvas
            effectsManager.UpdateCurrentEffect(*canvas, 16ms); // Assume ~60 FPS (delta time = 1/60)

            // Send the data to each feature's SocketChannel
            for (const auto &feature : canvas->Features())
            {
                auto pixelData = feature->GetPixelData();
                auto channel = socketController.FindChannelByHost(feature->HostName());
                if (channel)
                    channel->EnqueueFrame(pixelData, startTime);
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
