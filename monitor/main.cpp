#include <ncurses.h>
#include <curl/curl.h>
#include <chrono>
#include <thread>
#include <sstream>
#include <iomanip>
#include <vector>
#include <string>
#include <unistd.h> // for getopt
#include <cstdlib>  // for exit

// Our only interface to NDSCPP comes through the REST api and this serialization helper code
#include "../serialization.h"
#include "../interfaces.h"

#include "monitor.h"

using json = nlohmann::json;

void print_usage(const char *program_name)
{
    fprintf(stderr, "Usage: %s [-s hostname] [-p port]\n", program_name);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -s <hostname>  Specify the hostname to connect to (default: localhost)\n");
    fprintf(stderr, "  -p <port>      Specify the port to connect to (default: 7777)\n");
}

int main(int argc, char *argv[])
{
    std::string hostname = "localhost"; // default hostname
    int port = 7777;                    // default port
    int opt;

    // Parse command line options
    while ((opt = getopt(argc, argv, "s:p:h")) != -1)
    {
        switch (opt)
        {
        case 's':
            hostname = optarg;
            break;
        case 'p':
            try
            {
                port = std::stoi(optarg);
                if (port <= 0 || port > 65535)
                {
                    fprintf(stderr, "Error: Port must be between 1 and 65535\n");
                    exit(1);
                }
            }
            catch (const std::exception &e)
            {
                fprintf(stderr, "Error: Invalid port number\n");
                exit(1);
            }
            break;
        case 'h':
            print_usage(argv[0]);
            exit(0);
        default:
            print_usage(argv[0]);
            exit(1);
        }
    }

    curl_global_init(CURL_GLOBAL_ALL);

    // Pass hostname and port to Monitor constructor
    Monitor monitor(hostname, port);
    monitor.run();

    curl_global_cleanup();
    return 0;
}