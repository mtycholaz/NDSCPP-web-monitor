#include <csignal>
#include <iostream>
#include <atomic>
#include <chrono>
#include <thread>
#include "global.h"
#include "server.h"

using namespace std;

WebServer  webServer;

// Atomic flag to indicate whether the program should continue running. Will
// be set to false when SIGINT is received.

atomic<bool> running(true);

void handle_signal(int signal) 
{
    if (signal == SIGINT) 
    {
        running = false;
        cerr << "Received SIGINT, exiting...\n" << flush;
    }
}

// Main program entry point.  Runs the webServer and updates the known symbols list every hour.
// When SIGINT is received, exits gracefully.

int main(int, char *[])
{
    // Register signal handler for SIGINT
    signal(SIGINT, handle_signal);

    auto thread = webServer.Start();
    if (!thread)
    {
        cerr << "Failed to start the server thread\n";
        return 1;
    }
    cout << "Started server, waiting..." << endl;

    while (running) 
    {
        for (int i = 0; i < 3600 && running; ++i) 
            this_thread::sleep_for(chrono::seconds(1)); // Sleep in one-second increments
    }
    
    cout << "Stopping server..." << endl;
    webServer.Stop();
}
