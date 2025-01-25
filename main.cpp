#include "widget.h"
#include "ui_widget.h"
#include <chrono>
#include "date/date.h"
#include "libcron/Cron.h"
#include <thread>
#include <string>
#include <regex>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>

#include <QApplication>
#include <QTcpSocket>

using namespace libcron;
using namespace date;

#define TITLE_FORMAT  "Reminder - %b %e %I:%M %p"
#define DELAY_SECONDS 60 // check every 60 seconds
#define MAX_ENTRIES   100 // allow up to 100 entries in reminder file
#define DEBUG_TEST    false

// test if the current date/time match the event schedule
bool test(const std::string& schedule, const time_t from, const time_t now)
{
    if (DEBUG_TEST) std::cout << "calling test: From: " << from << " Now: " << now << std::endl ;
    auto c = CronData::create("* "+schedule); // libcron has seconds in first position
    bool res = c.is_valid();
    if (res) {
        const CronSchedule sched(c);
        tm tm{};
        localtime_r(&from, &tm);
        const time_t diff = now - from;
        for (long int i = DELAY_SECONDS; i <= diff; i += DELAY_SECONDS) {
            const time_t curr_time = from + i + tm.tm_gmtoff;
            const auto curr = std::chrono::system_clock::from_time_t(curr_time);
            if (sched.check(curr)) {
                if (DEBUG_TEST) std::cout << std::endl << "Success: " << schedule << std::endl << "Now: " << curr << std::endl ;
                return true;
            } else if (DEBUG_TEST) std::cout << std::endl << "Failed: " << schedule << std::endl << "Now: " << curr << std::endl;
        }
    } else {
        std::cerr << std::endl << "Error: schedule " << schedule << " cannot be parsed" << std::endl;
    }

    return false;
}

void show_window(const char *title, const char* background_color, const char* color, const std::string text)
{
    Widget *w;
    Ui_Widget *ui;
    QPalette pal;

    w = new Widget();
    w->setWindowTitle(title);
    ui = w->ui;
    pal = w->palette();
    pal.setColor(QPalette::Window, background_color);
    w->setPalette(pal);
    w->setAutoFillBackground(true); 

    // You can either modify the box position and dimensions in widget.ui or dynamically using:
    // w->setGeometry(x, y, width, height);

    QFont font = w->font();
    font.setPointSize(24);
    ui->label->setFont(font);
    ui->label->setText(("<span style=\"color:"+std::string(color)+"\">"+text+"</span>").c_str());

    w->show();
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    std::string text = "";
    const char *background_color = "yellow";
    const char *color = "black";
    time_t now = time(0);
    time_t last_processed = now - DELAY_SECONDS;
    tm* local = localtime(&now);
    struct stat fs;
    char title[30];
    int num_lines = 0;
    std::string line;
    std::string lines[MAX_ENTRIES];
    strftime(title, 30, TITLE_FORMAT, local);

    if (argc <= 2) {
        const char *fn;
        if (argc == 2) {
            fn = argv[1];
            if (stat(fn, &fs)) {
                std::cerr << std::endl << "Error: failed to open " << fn << std::endl;
                exit(1);
            }
        } else {
            std::string buf;
            buf = std::string(std::getenv("HOME")) + std::string("/.reminders");
            fn = strdup(buf.c_str());
        }

        // zero out seconds on initial last_processed time
        local = localtime(&last_processed);
        local->tm_sec = 0;
        last_processed = mktime(local);

        // main loop wakes up every DELAY_SECONDS to check if any events have fired
        time_t last_mtime = 0;
        bool shown = false;
        while(true) {
            if (! shown) {
                // sleep until seconds == 0
                now = time(0);
                local = localtime(&now);
                if ((DELAY_SECONDS - local->tm_sec) > 0) {
                    if (DEBUG_TEST) std::cout << "sleeping: " << (DELAY_SECONDS - local->tm_sec) << std::endl ;
                    std::this_thread::sleep_for(std::chrono::seconds(DELAY_SECONDS - local->tm_sec));
                }
                if (DEBUG_TEST) std::cout << "starting" << std::endl;
            }
            now = time(0);
            local = localtime(&now);
            local->tm_sec = 0;
            now = mktime(local); // force second to 0
            strftime(title, 30, TITLE_FORMAT, local);
            if (stat(fn, &fs) == 0) {
                // if the file has changed, reread it into memory
                if (fs.st_mtim.tv_sec != last_mtime) {
                    std::ifstream infile(fn);
                    num_lines = 0;
                    while (std::getline(infile, line)) {
                        if (line.length() > 0 && line[0] != '#') {
                            if (num_lines < MAX_ENTRIES) {
                                lines[num_lines] = line;
                                num_lines++;
                            } else {
                                std::cerr << std::endl << "Error: MAX_ENTRIES exceeded. Ignoring " << line << std::endl;
                            }
                        }
                    }
                    last_mtime = fs.st_mtim.tv_sec;
                    infile.close();
                }
                // test all scheduled events and display if time matches
                shown = false;
                for(int current_line = 0; current_line < num_lines; current_line++) {
                    line = lines[current_line];
                    std::regex pattern{ R"#(^\s*([^\s]*?\s+[^\s]*?\s+[^\s]*?\s+[^\s]*?\s+[^\s]*?)\s+(.*?)\s+(.*?)\s+(.*?)\s*$)#" };
                    std::smatch match;

                    if (line[0] != '#' && std::regex_match(line, match, pattern)) {
                        if (DEBUG_TEST) std::cout << std::endl << "sched: " << match[1].str() << std::endl << "background: " << match[2].str() << std::endl << "color: " << match[3].str() << std::endl << "text: " << match[4].str() << std::endl;
                        if (test(match[1].str().c_str(), last_processed, now)) {
                            if (strlen(match[2].str().c_str()) > 1) {
                                background_color = strdup(match[2].str().c_str());
                            } else {
                                background_color = "yellow";
                            }
                            if (strlen(match[3].str().c_str()) > 1) {
                                color = match[3].str().c_str();
                            } else {
                                color = "black";
                            }
                            text = match[4].str();
                            show_window(title, background_color, color, text);
                            shown = true;
                        }
                    }
                }
                if (shown) {
                    // this call blocks processing so need to test() from last_processed to now
                    a.exec();
                }
            } else {
                std::cerr << std::endl << "Error: failed to open " << fn << std::endl;
                exit(1);
            }
            last_processed = now;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } else {
        if (strlen(argv[1]) > 1) {
            background_color = argv[1];
        }
        if (strlen(argv[2]) > 1) {
            color = argv[2];
        }
        for (int i = 3; i < argc; i++) {
            text += std::string(argv[i]);
            text += " ";
        }
        show_window(title, background_color, color, text);
        a.exec();
    }
    return 0;
}
