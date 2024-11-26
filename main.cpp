// Main.cpp
// 
// This file is the main entry point for the NDSCPP LED Matrix Server application.
// It creates a Canvas, adds a GreenFillEffect to it, and then enters a loop where it
// renders the effect to the canvas, compresses the data, and sends it to the LED
// matrix via a SocketChannel.  The program will continue to run until it receives
// a SIGINT signal (Ctrl-C).

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
#include "greenfilleffect.h"
#include "starfield.h"
#include "videoeffect.h"
#include "misceffects.h"

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

    // Define a Canvas for the Mesmerizer

    auto canvasMesmerizer = make_unique<Canvas>(64, 32, 20);
    auto feature1 = make_unique<LEDFeature>(
        canvasMesmerizer.get(),         // Canvas pointer
        "192.168.8.161",      // Hostname
        "Mesmerizer",         // Friendly Name
        49152,                // Port
        64, 32,               // Width, Height
        0, 0,                 // Offset X, Offset Y
        false,                // Reversed
        0,                    // Channel
        false,                // Red-Green Swap
        180                    // Client Buffer Count    
    );
    canvasMesmerizer->AddFeature(std::move(feature1));
    canvasMesmerizer->Effects().AddEffect(make_unique<MP4PlaybackEffect>("Starfield", "./media/mp4/rickroll.mp4"));
    canvasMesmerizer->Effects().SetCurrentEffect(0, *canvasMesmerizer);
    canvases.push_back(std::move(canvasMesmerizer));

    //---------------------------------------------------------------------

    // Define a Canvas for the Workbench Banner

    auto canvasBanner = make_unique<Canvas>(512, 32, 24);
    auto featureBanner = make_unique<LEDFeature>(
        canvasBanner.get(),   // Canvas pointer
        "192.168.1.98",       // Hostname
        "Banner",             // Friendly Name
        49152,                // Port∏
        512, 32,              // Width, Height
        0, 0,                 // Offset X, Offset Y
        false,                // Reversed
        0,                    // Channel
        false,                 // Red-Green Swap
        300
    );
    canvasBanner->AddFeature(std::move(featureBanner));
    canvasBanner->Effects().AddEffect(make_unique<StarfieldEffect>("Starfield", 100));
    canvasBanner->Effects().SetCurrentEffect(0, *canvasBanner);
    canvases.push_back(std::move(canvasBanner));

    //---------------------------------------------------------------------

    auto canvasWindow1 = make_unique<Canvas>(100, 1, 1);
    auto featureWindow1 = make_unique<LEDFeature>(
        canvasWindow1.get(),  // Canvas pointer
        "192.168.8.8",        // Hostname
        "Window1",            // Friendly Name
        49152,                // Port∏
        100, 1,               // Width, Height
        0, 0,                 // Offset X, Offset Y
        false,                // Reversed
        0,                    // Channel
        false,                // Red-Green Swap
        21
    );
    canvasWindow1->AddFeature(std::move(featureWindow1));
    canvasWindow1->Effects().AddEffect(make_unique<SolidColorFill>("Yellow Window", CRGB(255, 112, 0)));
    canvasWindow1->Effects().SetCurrentEffect(0, *canvasWindow1);
    canvases.push_back(std::move(canvasWindow1));
 
    //---------------------------------------------------------------------

    auto canvasWindow2 = make_unique<Canvas>(100, 1, 1);
    auto featureWindow2 = make_unique<LEDFeature>(
        canvasWindow2.get(),  // Canvas pointer
        "192.168.8.9",        // Hostname
        "Window1",            // Friendly Name
        49152,                // Port∏
        100, 1,               // Width, Height
        0, 0,                 // Offset X, Offset Y
        false,                // Reversed
        0,                    // Channel
        false,                // Red-Green Swap
        21
    );
    canvasWindow2->AddFeature(std::move(featureWindow2));
    canvasWindow2->Effects().AddEffect(make_unique<SolidColorFill>("Blue Window", CRGB::Blue));
    canvasWindow2->Effects().SetCurrentEffect(0, *canvasWindow2);
    canvases.push_back(std::move(canvasWindow2));

    //---------------------------------------------------------------------

    auto canvasWindow3 = make_unique<Canvas>(100, 1, 1);
    auto featureWindow3 = make_unique<LEDFeature>(
        canvasWindow3.get(),  // Canvas pointer
        "192.168.8.10",       // Hostname
        "Window1",            // Friendly Name
        49152,                // Port∏
        100, 1,               // Width, Height
        0, 0,                 // Offset X, Offset Y
        false,                // Reversed
        0,                    // Channel
        false,                // Red-Green Swap
        21
    );
    canvasWindow3->AddFeature(std::move(featureWindow3));
    canvasWindow3->Effects().AddEffect(make_unique<SolidColorFill>("Green Window", CRGB::Green));
    canvasWindow3->Effects().SetCurrentEffect(0, *canvasWindow3);
    canvases.push_back(std::move(canvasWindow3));

    //---------------------------------------------------------------------

    constexpr auto start1 = 0, length1 = 300+200;
    constexpr auto start2 = length1, length2 = 300+300;
    constexpr auto start3 = length1+length2, length3 = 144;
    constexpr auto start4 = length1+length2+length3, length4 = 144;
    constexpr auto totalLength = length1 + length2 + length3 + length4;

    auto canvasCabinets = make_unique<Canvas>(totalLength, 1, 20);
    auto featureCabinets1 = make_unique<LEDFeature>(
        canvasCabinets.get(), // Canvas pointer
        "192.168.8.12",       // Hostname
        "Cupboard1",          // Friendly Name
        49152,                // Port∏
        length1, 1,           // Width, Height
        start1, 0,            // Offset X, Offset Y
        false,                // Reversed
        0,                    // Channel
        false,                // Red-Green Swap
        21
    );
    auto featureCabinets2 = make_unique<LEDFeature>(
        canvasCabinets.get(), // Canvas pointer
        "192.168.8.29",       // Hostname
        "Cupboard1",          // Friendly Name
        49152,                // Port∏
        length2, 1,           // Width, Height
        start2, 0,            // Offset X, Offset Y
        false,                // Reversed
        0,                    // Channel
        false,                // Red-Green Swap
        21
    );
    auto featureCabinets3 = make_unique<LEDFeature>(
        canvasCabinets.get(), // Canvas pointer
        "192.168.8.30",       // Hostname
        "Cupboard1",          // Friendly Name
        49152,                // Port∏
        length3, 1,           // Width, Height
        start3, 0,            // Offset X, Offset Y
        false,                // Reversed
        0,                    // Channel
        false,                // Red-Green Swap
        21
    );
    auto featureCabinets4 = make_unique<LEDFeature>(
        canvasCabinets.get(), // Canvas pointer
        "192.168.8.15",       // Hostname
        "Cupboard1",          // Friendly Name
        49152,                // Port∏
        length4, 1,           // Width, Height
        start4, 0,            // Offset X, Offset Y
        false,                // Reversed
        0,                    // Channel
        false,                // Red-Green Swap
        21
    );
    
    canvasCabinets->AddFeature(std::move(featureCabinets1));
    canvasCabinets->AddFeature(std::move(featureCabinets2));
    canvasCabinets->AddFeature(std::move(featureCabinets3));
    canvasCabinets->AddFeature(std::move(featureCabinets4));
    canvasCabinets->Effects().AddEffect(make_unique<SolidColorFill>("Green Test", CRGB::Green));
    canvasCabinets->Effects().SetCurrentEffect(0, *canvasCabinets);
    canvases.push_back(std::move(canvasCabinets));

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

    const auto allCanvases = LoadCanvases();

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
    webServer.Start();
    
    cout << "Shutting down..." << endl;

    running = false;

    // Shut down rendering and communications

    cout << "Stopping Effects..." << endl;
    for (const auto &canvas : allCanvases)
         canvas->Effects().Stop();

    cout << "Stopping Sockets..." << endl;
    for (const auto &canvas : allCanvases)
        for (auto &feature : canvas->Features())
            feature->Socket().Stop();

    cout << "Shut down complete." << endl;
    
    return 0;
}
