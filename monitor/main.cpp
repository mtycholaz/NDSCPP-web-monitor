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

std::string formatTimeDelta(int64_t delta)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << (delta / 1000.0) << "s";
    return oss.str();
}

// Screen layout constants
constexpr int HEADER_HEIGHT = 3;
constexpr int FOOTER_HEIGHT = 2;

// Updated columns with new information
const std::vector<std::pair<std::string, int>> COLUMNS = 
{
    {"Can", 3},          // Canvas ID
    {"Feature", 12},     // Feature name
    {"Host", 14},        // Hostname
    {"Size", 8},         // Dimensions
    {"FPS", 4},          // Frames per second
    {"Queue", 7},        // Queue depth
    {"Buf", 7},          // Buffer usage
    {"Signal", 8},       // WiFi signal
    {"B/W", 9},          // Bandwidth
    {"Delta", 7},        // Clock delta
    {"Seq", 6},          // Sequence number
    {"Flash", 5},        // Flash version
    {"Status", 6}        // Connection status
};

class Monitor
{
    WINDOW *headerWin;
    WINDOW *contentWin;
    WINDOW *footerWin;
    int contentHeight;
    int scrollOffset = 0;
    std::string baseUrl = "http://m2macpro:7777";

public:
    Monitor()
    {
        initscr();
        start_color();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        curs_set(0);
        refresh();

        init_pair(1, COLOR_GREEN, COLOR_BLACK);   // Connected
        init_pair(2, COLOR_RED, COLOR_BLACK);     // Disconnected
        init_pair(3, COLOR_YELLOW, COLOR_BLACK);  // Headers
        init_pair(4, COLOR_CYAN, COLOR_BLACK);    // Highlights

        int maxY, maxX;
        getmaxyx(stdscr, maxY, maxX);
        contentHeight = maxY - HEADER_HEIGHT - FOOTER_HEIGHT;

        headerWin = newwin(HEADER_HEIGHT, maxX, 0, 0);
        contentWin = newwin(contentHeight, maxX, HEADER_HEIGHT, 0);
        footerWin = newwin(FOOTER_HEIGHT, maxX, maxY - FOOTER_HEIGHT, 0);

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
            std::string response = httpGet(baseUrl + "/api/canvases");
            auto j = json::parse(response);
            auto currentTime = std::chrono::system_clock::now().time_since_epoch().count() / 1000000; // Current time in ms

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
                        std::string featureName = featureJson["friendlyName"].get<std::string>();
                        if (featureName.length() > (size_t) COLUMNS[1].second)
                            featureName = featureName.substr(0, COLUMNS[1].second);
                        mvwprintw(contentWin, row - scrollOffset, x, "%-*s",
                                COLUMNS[1].second, featureName.c_str());
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

                        bool isConnected = featureJson["isConnected"].get<bool>();

                        // FPS (only if connected and has client response)
                        if (isConnected && featureJson.contains("lastClientResponse"))
                        {
                            const auto &stats = featureJson["lastClientResponse"];
                            mvwprintw(contentWin, row - scrollOffset, x, "%-*d",
                                    COLUMNS[4].second, stats["fpsDrawing"].get<int>());
                        }
                        else
                        {
                            mvwprintw(contentWin, row - scrollOffset, x, "%-*s",
                                    COLUMNS[4].second, "---");
                        }
                        x += COLUMNS[4].second + 1;

                        // Queue depth (when available from server)
                        if (!featureJson.is_null() && featureJson.contains("queueDepth") && featureJson.contains("queueMaxSize")) {
                            try {
                                std::string queueStatus = std::to_string(featureJson["queueDepth"].get<size_t>()) + "/" +
                                                        std::to_string(featureJson["queueMaxSize"].get<size_t>());
                                mvwprintw(contentWin, row - scrollOffset, x, "%-*s",
                                        COLUMNS[5].second, queueStatus.c_str());
                            } catch (const json::exception&) {
                                mvwprintw(contentWin, row - scrollOffset, x, "%-*s",
                                        COLUMNS[5].second, "---");
                            }
                        } else {
                            mvwprintw(contentWin, row - scrollOffset, x, "%-*s",
                                    COLUMNS[5].second, "---");
                        }
                        x += COLUMNS[5].second + 1;


                        // Rest of the columns (only if connected and has client response)
                        if (isConnected && featureJson.contains("lastClientResponse"))
                        {
                            const auto &stats = featureJson["lastClientResponse"];

                            // Buffer usage
                            std::string buffer = std::to_string(stats["bufferPos"].get<size_t>()) + "/" +
                                            std::to_string(stats["bufferSize"].get<size_t>());
                            mvwprintw(contentWin, row - scrollOffset, x, "%-*s",
                                    COLUMNS[6].second, buffer.c_str());
                            x += COLUMNS[6].second + 1;

                            // WiFi Signal
                            mvwprintw(contentWin, row - scrollOffset, x, "%-*s",
                                    COLUMNS[7].second, formatWifiSignal(stats["wifiSignal"].get<double>()).c_str());
                            x += COLUMNS[7].second + 1;

                            // Bandwidth
                            mvwprintw(contentWin, row - scrollOffset, x, "%-*s",
                                    COLUMNS[8].second, formatBytes(featureJson["bytesPerSecond"].get<double>()).c_str());
                            x += COLUMNS[8].second + 1;

                            // Clock Delta
                            if (stats.contains("currentClock")) {
                                try {
                                    int64_t clockDelta = stats["currentClock"].get<int64_t>() - currentTime;
                                    mvwprintw(contentWin, row - scrollOffset, x, "%-*s",
                                            COLUMNS[9].second, formatTimeDelta(clockDelta).c_str());
                                } catch (const json::exception&) {
                                    mvwprintw(contentWin, row - scrollOffset, x, "%-*s",
                                            COLUMNS[9].second, "---");
                                }
                            } else {
                                mvwprintw(contentWin, row - scrollOffset, x, "%-*s",
                                        COLUMNS[9].second, "---");
                            }
                            x += COLUMNS[9].second + 1;

                            // Sequence Number
                            if (stats.contains("sequenceNumber")) {
                                try {
                                    int seqNum = stats["sequenceNumber"].get<int>();
                                    mvwprintw(contentWin, row - scrollOffset, x, "%-*d",
                                            COLUMNS[10].second, seqNum);
                                } catch (const json::exception&) {
                                    mvwprintw(contentWin, row - scrollOffset, x, "%-*s",
                                            COLUMNS[10].second, "---");
                                }
                            } else {
                                mvwprintw(contentWin, row - scrollOffset, x, "%-*s",
                                        COLUMNS[10].second, "---");
                            }
                            x += COLUMNS[10].second + 1;

                            // Flash Version
                            if (stats.contains("flashVersion")) {
                                try {
                                    std::string flashVer;
                                    if (stats["flashVersion"].is_string()) {
                                        flashVer = stats["flashVersion"].get<std::string>();
                                    } else if (stats["flashVersion"].is_number()) {
                                        flashVer = std::to_string(stats["flashVersion"].get<int>());
                                    } else {
                                        flashVer = "---";
                                    }
                                    mvwprintw(contentWin, row - scrollOffset, x, "%-*s",
                                            COLUMNS[11].second, flashVer.c_str());
                                } catch (const json::exception&) {
                                    mvwprintw(contentWin, row - scrollOffset, x, "%-*s",
                                            COLUMNS[11].second, "---");
                                }
                            } else {
                                mvwprintw(contentWin, row - scrollOffset, x, "%-*s",
                                        COLUMNS[11].second, "---");
                            }
                            x += COLUMNS[11].second + 1;

                            // Connection status with color
                            wattron(contentWin, COLOR_PAIR(1));
                            mvwprintw(contentWin, row - scrollOffset, x, "%-*s",
                                    COLUMNS[12].second, "ONLINE");
                            wattroff(contentWin, COLOR_PAIR(1));
                        }
                        else
                        {
                            // Fill empty spaces for offline devices
                            for (size_t i = 6; i <= 11; i++) {
                                mvwprintw(contentWin, row - scrollOffset, x, "%-*s",
                                        COLUMNS[i].second, "---");
                                x += COLUMNS[i].second + 1;
                            }

                            // Connection status with color
                            wattron(contentWin, COLOR_PAIR(2));
                            mvwprintw(contentWin, row - scrollOffset, x, "%-*s",
                                    COLUMNS[12].second, "OFFLINE");
                            wattroff(contentWin, COLOR_PAIR(2));
                        }
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
        // Make content window non-blocking
        nodelay(contentWin, TRUE);
        
        bool running = true;
        auto lastUpdate = std::chrono::steady_clock::now();
        
        while (running)
        {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate);
            
            // Update display every 100ms
            if (elapsed.count() >= 100) {
                drawHeader();
                drawContent();
                drawFooter();
                lastUpdate = now;
            }

            int ch = wgetch(contentWin);
            if (ch != ERR) {  // Only process if we actually got input
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
                        lastUpdate = std::chrono::steady_clock::time_point::min(); // Force immediate refresh
                        break;
                }
            }
            
            // Small sleep to prevent CPU spinning
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
};

int main(int, char *[])
{
    curl_global_init(CURL_GLOBAL_ALL);
    Monitor monitor;
    monitor.run();
    curl_global_cleanup();
    return 0;
}