#include <ncurses.h>
#include <curl/curl.h>
#include <chrono>
#include <thread>
#include <sstream>
#include <iomanip>
#include <vector>
#include <string>

#include "../serialization.h"
#include "../interfaces.h"

using json = nlohmann::json;

// Updated columns with new information
extern const std::vector<std::pair<std::string, int>> COLUMNS;

// Screen layout constants
constexpr int HEADER_HEIGHT = 3;
constexpr int FOOTER_HEIGHT = 2;

class Monitor
{
    WINDOW *headerWin;
    WINDOW *contentWin;
    WINDOW *footerWin;
    int contentHeight;
    int scrollOffset = 0;
    std::string baseUrl;
    double _fps;

public:
    Monitor(const std::string& hostname = "localhost", int port = 7777, double fps = 10.0)
        : baseUrl(std::string("http://") + hostname + ":" + std::to_string(port)),
          _fps(fps)
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
        init_pair(5, COLOR_RED, COLOR_BLACK);     // High delta
        init_pair(6, COLOR_YELLOW, COLOR_BLACK);  // Warning level

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
        mvwprintw(headerWin, 0, 2, " NightDriver Monitor ");

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

    void drawContent();

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
            std::this_thread::sleep_for(std::chrono::milliseconds(_fps > 0 ? static_cast<int>(1000.0 / _fps) : 100));
        }
    }
};