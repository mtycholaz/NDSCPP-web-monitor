#include <ncurses.h>
#include <curl/curl.h>
#include <chrono>
#include <thread>
#include <sstream>
#include <iomanip>
#include <vector>
#include <string>

// Our only interface to NDSCPP comes through the REST api and this serialization helper code
#include "../serialization.h"
#include "../interfaces.h"

#include "monitor.h"

using json = nlohmann::json;

int main(int, char *[])
{
    curl_global_init(CURL_GLOBAL_ALL);
    Monitor monitor;
    monitor.run();
    curl_global_cleanup();
    return 0;
}