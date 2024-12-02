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

// Callback for CURL to write response data
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

std::string buildMeter(double value, double threshold, int width = 21)
{
    // Width should be odd to have a center point
    if (width % 2 == 0)
        width++;

    // Normalize value to -1..1 range based on threshold
    double normalized = std::min(1.0, std::max(-1.0, value / threshold));

    // Calculate position (center = width/2)
    int center = width / 2;
    int pos = center + static_cast<int>(normalized * center);

    std::string meter;
    for (int i = 0; i < width; i++)
    {
        meter += (i == pos) ? "|" : "-";
    }
    return meter;
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
    oss << std::fixed << std::setprecision(0) << bytes << units[unit]; // Removed decimal places
    return oss.str();
}

std::string formatWifiSignal(double signal)
{
    std::ostringstream oss;
    if (signal == 1000)
        oss << "1Gb/s";
    else if (signal == 10000)
        oss << "10Gb/s";
    else
        oss << std::abs((int)signal) << "dBm"; // Added abs() and removed decimal places
    return oss.str();
}

std::string formatTimeDelta(double delta)
{
    string meter = buildMeter(delta, 3.0, 5);
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << delta << "s " << meter;
    return oss.str();
}

const std::vector<std::pair<std::string, int>> COLUMNS =
    {
        {"Canvas", 10},  // Canvas name (matched to Feature width)
        {"Feature", 10}, // Feature name
        {"Host", 14},    // Hostname
        {"Size", 7},     // Dimensions
        {"FPS", 6},      // Frames per second (X/Y format)
        {"Queue", 5},    // Queue depth
        {"Buf", 8},      // Buffer usage
        {"Signal", 6},   // WiFi signal
        {"Data", 7},     // Bandwidth
        {"Delta", 12},   // Clock delta
        {"Flash", 5},    // Flash version
        {"Status", 6}    // Connection status
};
void Monitor::drawContent()
{
    werase(contentWin);

    try
    {
        std::string response = httpGet(baseUrl + "/api/canvases");
        auto j = json::parse(response);
        auto currentTime = std::chrono::system_clock::now().time_since_epoch().count() / 1000000.0; // Current time in ms

        int row = 0;
        for (const auto &canvasJson : j)
        {
            std::string canvasName = canvasJson["name"].get<std::string>();
            int canvasFps = canvasJson["fps"].get<int>();

            for (const auto &featureJson : canvasJson["features"])
            {
                if (row >= scrollOffset && row < scrollOffset + contentHeight)
                {
                    int x = 1;

                    // Canvas name
                    std::string displayName = canvasName;
                    if (displayName.length() > (size_t)COLUMNS[0].second)
                        displayName = displayName.substr(0, COLUMNS[0].second);
                    wattron(contentWin, A_BOLD | COLOR_PAIR(4)); // Assuming 5 is your cyan color pair
                    mvwprintw(contentWin, row - scrollOffset, x, "%-*s", COLUMNS[0].second, displayName.c_str());
                    wattroff(contentWin, A_BOLD | COLOR_PAIR(4));
                    x += COLUMNS[0].second + 1;

                    // Feature name
                    std::string featureName = featureJson["friendlyName"].get<std::string>();
                    if (featureName.length() > (size_t)COLUMNS[1].second)
                        featureName = featureName.substr(0, COLUMNS[1].second);
                    mvwprintw(contentWin, row - scrollOffset, x, "%-*s", COLUMNS[1].second, featureName.c_str());
                    x += COLUMNS[1].second + 1;

                    // Hostname
                    mvwprintw(contentWin, row - scrollOffset, x, "%-*s", COLUMNS[2].second, featureJson["hostName"].get<std::string>().c_str());
                    x += COLUMNS[2].second + 1;

                    // Size
                    std::string size = std::to_string(featureJson["width"].get<size_t>()) + "x" +
                                       std::to_string(featureJson["height"].get<size_t>());
                    mvwprintw(contentWin, row - scrollOffset, x, "%-*s",
                              COLUMNS[3].second, size.c_str());
                    x += COLUMNS[3].second + 1;

                    bool isConnected = featureJson["isConnected"].get<bool>();

                    // FPS (only if connected and has client response)
                    // FPS (only if connected and has client response)
                    if (isConnected && featureJson.contains("lastClientResponse"))
                    {
                        const auto &stats = featureJson["lastClientResponse"];
                        int featureFps = stats["fpsDrawing"].get<int>();
                        
                        // Color the numerator based on performance
                        int colorPair = (featureFps < 0.8 * canvasFps) ? 6 : 1;
                        wattron(contentWin, COLOR_PAIR(colorPair));
                        mvwprintw(contentWin, row - scrollOffset, x, "%d", featureFps);
                        wattroff(contentWin, COLOR_PAIR(colorPair));
                        
                        // Print the slash and denominator in default color
                        mvwprintw(contentWin, row - scrollOffset, x + std::to_string(featureFps).length(), "/%d", canvasFps);
                        
                        // Pad with spaces to fill column width
                        int totalWidth = std::to_string(featureFps).length() + 1 + std::to_string(canvasFps).length();
                        mvwprintw(contentWin, row - scrollOffset, x + totalWidth, "%*s", COLUMNS[4].second - totalWidth, "");
                    }
                    else
                    {
                        mvwprintw(contentWin, row - scrollOffset, x, "%-*s", COLUMNS[4].second, "---");
                    }
                    x += COLUMNS[4].second + 1;

                    // Queue depth (when available from server)
                    if (featureJson.contains("queueDepth") && !featureJson["queueDepth"].is_null())
                    {
                        try
                        {
                            size_t queueDepth = featureJson["queueDepth"].get<size_t>();
                            int queueColor;
                            if (queueDepth < 100)
                                queueColor = 1; // Green for low queue
                            else if (queueDepth < 250)
                                queueColor = 6; // Yellow for medium queue
                            else
                                queueColor = 2; // Red for high queue

                            wattron(contentWin, COLOR_PAIR(queueColor));
                            mvwprintw(contentWin, row - scrollOffset, x, "%-*zu", COLUMNS[5].second, queueDepth);
                            wattroff(contentWin, COLOR_PAIR(queueColor));
                        }
                        catch (const json::exception &)
                        {
                            mvwprintw(contentWin, row - scrollOffset, x, "%-*s", COLUMNS[5].second, "---");
                        }
                    }
                    else
                    {
                        mvwprintw(contentWin, row - scrollOffset, x, "%-*s", COLUMNS[5].second, "---");
                    }
                    x += COLUMNS[5].second + 1;

                    // Rest of the columns (only if connected and has client response)
                    if (isConnected && featureJson.contains("lastClientResponse"))
                    {
                        const auto &stats = featureJson["lastClientResponse"];

                        // Buffer usage with color coding
                        size_t bufferPos = stats["bufferPos"].get<size_t>();
                        size_t bufferSize = stats["bufferSize"].get<size_t>();
                        double ratio = static_cast<double>(bufferPos) / bufferSize;

                        // Determine color based on ratio ranges
                        int colorPair;
                        if (ratio >= 0.25 && ratio <= 0.85)
                            colorPair = 1; // green
                        else if (ratio > 0.95)
                            colorPair = 2; // red
                        else
                            colorPair = 3; // yellow for outside optimal range but not critical

                        // First part with color
                        wattron(contentWin, COLOR_PAIR(colorPair));
                        mvwprintw(contentWin, row - scrollOffset, x, "%zu", bufferPos);
                        wattroff(contentWin, COLOR_PAIR(colorPair));

                        // Second part without color
                        mvwprintw(contentWin, row - scrollOffset, x + std::to_string(bufferPos).length(), "/%zu", bufferSize);
                        x += COLUMNS[6].second + 1;

                        // WiFi Signal with color coding
                        double signal = std::abs(stats["wifiSignal"].get<double>());
                        int signalColor;
                        if (signal >= 100)
                            signalColor = 0;
                        else if (signal < 70)
                            signalColor = 1; // Green for good signal
                        else if (signal < 80)
                            signalColor = 6; // Yellow for warning
                        else
                            signalColor = 2; // Red for poor signal

                        wattron(contentWin, COLOR_PAIR(signalColor));
                        mvwprintw(contentWin, row - scrollOffset, x, "%-*s", COLUMNS[7].second, formatWifiSignal(stats["wifiSignal"].get<double>()).c_str());
                        wattroff(contentWin, COLOR_PAIR(signalColor));
                        x += COLUMNS[7].second + 1;

                        // Bandwidth
                        mvwprintw(contentWin, row - scrollOffset, x, "%-*s", COLUMNS[8].second, formatBytes(featureJson["bytesPerSecond"].get<double>()).c_str());
                        x += COLUMNS[8].second + 1;

                        // Clock Delta with color
                        if (stats.contains("currentClock"))
                        {
                            try
                            {
                                double clockDelta = stats["currentClock"].get<double>() - currentTime;
                                double deltaSeconds = std::abs(clockDelta);
                                int deltaColor;
                                if (deltaSeconds < 2.0)
                                    deltaColor = 1; // Green for low delta
                                else if (deltaSeconds < 3.0)
                                    deltaColor = 6; // Yellow for medium delta
                                else
                                    deltaColor = 3; // Red for high delta

                                std::string deltaStr = formatTimeDelta(clockDelta);
                                wattron(contentWin, COLOR_PAIR(deltaColor));
                                mvwprintw(contentWin, row - scrollOffset, x, "%-*s", COLUMNS[9].second, deltaStr.c_str());
                                wattroff(contentWin, COLOR_PAIR(deltaColor));
                            }
                            catch (const json::exception &)
                            {
                                mvwprintw(contentWin, row - scrollOffset, x, "%-*s", COLUMNS[9].second, "---");
                            }
                        }
                        else
                        {
                            mvwprintw(contentWin, row - scrollOffset, x, "%-*s", COLUMNS[9].second, "---");
                        }
                        x += COLUMNS[9].second + 1;

                        // Flash Version (with 'v' prefix)
                        if (stats.contains("flashVersion"))
                        {
                            try
                            {
                                std::string flashVer = "v";
                                if (stats["flashVersion"].is_string())
                                    flashVer += stats["flashVersion"].get<std::string>();
                                else if (stats["flashVersion"].is_number())
                                    flashVer += std::to_string(stats["flashVersion"].get<int>());
                                else
                                    flashVer = "---";
                                mvwprintw(contentWin, row - scrollOffset, x, "%-*s", COLUMNS[10].second, flashVer.c_str());
                            }
                            catch (const json::exception &)
                            {
                                mvwprintw(contentWin, row - scrollOffset, x, "%-*s", COLUMNS[10].second, "---");
                            }
                        }
                        else
                        {
                            mvwprintw(contentWin, row - scrollOffset, x, "%-*s", COLUMNS[10].second, "---");
                        }
                        x += COLUMNS[10].second + 1;

                        // Connection status with color
                        wattron(contentWin, COLOR_PAIR(1));
                        mvwprintw(contentWin, row - scrollOffset, x, "%-*s", COLUMNS[11].second, "ONLINE");
                        wattroff(contentWin, COLOR_PAIR(1));
                    }
                    else
                    {
                        // Fill empty spaces for offline devices
                        for (size_t i = 6; i <= 10; i++)
                        {
                            mvwprintw(contentWin, row - scrollOffset, x, "%-*s", COLUMNS[i].second, "---");
                            x += COLUMNS[i].second + 1;
                        }

                        // Connection status with color
                        wattron(contentWin, COLOR_PAIR(2));
                        mvwprintw(contentWin, row - scrollOffset, x, "%-*s", COLUMNS[11].second, "OFFLINE");
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