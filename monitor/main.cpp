// Quick and dirty ncurses-based monitor for the NDSCPP LED Matrix REST API server just to make sure
// it works and that we can serialize the data correctly.

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

using json = nlohmann::json;

// Callback for CURL to write response data
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

// Helper to make HTTP requests
std::string httpGet(const std::string &url)
{
    CURL *curl = curl_easy_init();
    std::string response;

    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK)
            response = "Error: " + std::string(curl_easy_strerror(res));

        curl_easy_cleanup(curl);
    }
    return response;
}

// Format helpers
std::string formatBytes(double bytes)
{
    // Let's be optimistic and assume we won't need to format more than GB/s
    const char *units[] = {"B/s", "KB/s", "MB/s", "GB/s"};
    int unit = 0;
    while (bytes >= 1024 && unit < 3)
    {
        bytes /= 1024;
        unit++;
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << bytes << units[unit];
    return oss.str();
}

std::string formatWifiSignal(double signal)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << signal << "dBm";
    return oss.str();
}

// Screen layout constants
constexpr int HEADER_HEIGHT = 3;
constexpr int FOOTER_HEIGHT = 2;

// Flashback to the original TaskManager prototype :-)
const std::vector<std::pair<std::string, int>> COLUMNS = 
{
    {"Canvas", 8},
    {"Feature", 15},
    {"Host", 16},
    {"Size", 12},
    {"FPS", 5},
    {"Buffer", 10},
    {"Signal", 8},
    {"B/W", 10},
    {"Status", 8}
};

class Monitor
{
    WINDOW *headerWin;
    WINDOW *contentWin;
    WINDOW *footerWin;
    int contentHeight;
    int scrollOffset = 0;
    std::string baseUrl = "http://localhost:7777";

public:

    Monitor()
    {
        // Initialize ncurses
        initscr();
        start_color();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        curs_set(0);
        refresh();

        // Set up colors
        init_pair(1, COLOR_GREEN, COLOR_BLACK);  // Connected
        init_pair(2, COLOR_RED, COLOR_BLACK);    // Disconnected
        init_pair(3, COLOR_YELLOW, COLOR_BLACK); // Headers
        init_pair(4, COLOR_CYAN, COLOR_BLACK);   // Highlights

        // Create windows
        int maxY, maxX;
        getmaxyx(stdscr, maxY, maxX);
        contentHeight = maxY - HEADER_HEIGHT - FOOTER_HEIGHT;

        headerWin = newwin(HEADER_HEIGHT, maxX, 0, 0);
        contentWin = newwin(contentHeight, maxX, HEADER_HEIGHT, 0);
        footerWin = newwin(FOOTER_HEIGHT, maxX, maxY - FOOTER_HEIGHT, 0);

        // Enable scrolling for content window
        scrollok(contentWin, TRUE);
        keypad(contentWin, TRUE);
    }

    ~Monitor()
    {
        delwin(headerWin);
        delwin(contentWin);
        delwin(footerWin);
        endwin();
    }

    void drawHeader()
    {
        werase(headerWin);
        box(headerWin, 0, 0);
        mvwprintw(headerWin, 0, 2, " LED Matrix Monitor ");

        // Draw column headers
        int x = 1;
        wattron(headerWin, COLOR_PAIR(3));
        for (const auto &col : COLUMNS)
        {
            mvwprintw(headerWin, 1, x, "%-*s", col.second, col.first.c_str());
            x += col.second + 1;
        }
        wattroff(headerWin, COLOR_PAIR(3));

        wrefresh(headerWin);
    }

    void drawContent()
    {
        werase(contentWin);

        try
        {
            // Fetch canvas data
            std::string response = httpGet(baseUrl + "/api/canvases");
            auto j = json::parse(response);

            int row = 0;
            for (const auto &canvasJson : j)
            {
                std::string canvasId = std::to_string(canvasJson["id"].get<size_t>());

                for (const auto &featureJson : canvasJson["features"])
                {
                    if (row >= scrollOffset && row < scrollOffset + contentHeight)
                    {
                        int x = 1;

                        // Canvas ID
                        mvwprintw(contentWin, row - scrollOffset, x, "%-*s",
                                  COLUMNS[0].second, canvasId.c_str());
                        x += COLUMNS[0].second + 1;

                        // Feature name
                        mvwprintw(contentWin, row - scrollOffset, x, "%-*s",
                                  COLUMNS[1].second, featureJson["friendlyName"].get<std::string>().c_str());
                        x += COLUMNS[1].second + 1;

                        // Hostname
                        mvwprintw(contentWin, row - scrollOffset, x, "%-*s",
                                  COLUMNS[2].second, featureJson["hostName"].get<std::string>().c_str());
                        x += COLUMNS[2].second + 1;

                        // Size
                        std::string size = std::to_string(featureJson["width"].get<size_t>()) + "x" +
                                           std::to_string(featureJson["height"].get<size_t>());
                        mvwprintw(contentWin, row - scrollOffset, x, "%-*s",
                                  COLUMNS[3].second, size.c_str());
                        x += COLUMNS[3].second + 1;

                        // Get client response data if available
                        if (featureJson.contains("lastClientResponse"))
                        {
                            const auto &stats = featureJson["lastClientResponse"];

                            // FPS
                            mvwprintw(contentWin, row - scrollOffset, x, "%-*d",
                                      COLUMNS[4].second, stats["fpsDrawing"].get<int>());
                            x += COLUMNS[4].second + 1;

                            // Buffer usage
                            std::string buffer = std::to_string(stats["bufferPos"].get<size_t>()) + "/" +
                                                 std::to_string(stats["bufferSize"].get<size_t>());
                            mvwprintw(contentWin, row - scrollOffset, x, "%-*s",
                                      COLUMNS[5].second, buffer.c_str());
                            x += COLUMNS[5].second + 1;

                            // WiFi Signal
                            mvwprintw(contentWin, row - scrollOffset, x, "%-*s",
                                      COLUMNS[6].second, formatWifiSignal(stats["wifiSignal"].get<double>()).c_str());
                            x += COLUMNS[6].second + 1;

                            // Bandwidth
                            mvwprintw(contentWin, row - scrollOffset, x, "%-*s",
                                      COLUMNS[7].second, formatBytes(featureJson["bytesPerSecond"].get<double>()).c_str());
                            x += COLUMNS[7].second + 1;
                        }

                        // Connection status with color
                        bool isConnected = featureJson["isConnected"].get<bool>();
                        wattron(contentWin, COLOR_PAIR(isConnected ? 1 : 2));
                        mvwprintw(contentWin, row - scrollOffset, x, "%-*s",
                                  COLUMNS[8].second, isConnected ? "ONLINE" : "OFFLINE");
                        wattroff(contentWin, COLOR_PAIR(isConnected ? 1 : 2));
                    }
                    row++;
                }
            }
        }
        catch (const std::exception &e)
        {
            mvwprintw(contentWin, 0, 1, "Error fetching data: %s", e.what());
        }

        wrefresh(contentWin);
    }

    void drawFooter()
    {
        werase(footerWin);
        box(footerWin, 0, 0);
        mvwprintw(footerWin, 0, 2, " Controls ");
        mvwprintw(footerWin, 1, 2, "Q:Quit  ↑/↓:Scroll  R:Refresh");
        wrefresh(footerWin);
    }

    void run()
    {
        bool running = true;

        while (running)
        {
            drawHeader();
            drawContent();
            drawFooter();

            int ch = wgetch(contentWin);
            switch (ch)
            {
            case 'q':
            case 'Q':
                running = false;
                break;
            case KEY_UP:
                if (scrollOffset > 0)
                    scrollOffset--;
                break;
            case KEY_DOWN:
                scrollOffset++;
                break;
            case 'r':
            case 'R':
                // Refresh will happen on next loop
                break;
            }

            // Add a small delay to prevent hammering the API
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
};

int main(int, char *[])
{
    // Initialize CURL
    curl_global_init(CURL_GLOBAL_ALL);

    // Run the monitor
    Monitor monitor;
    monitor.run();

    // Cleanup
    curl_global_cleanup();
    return 0;
}