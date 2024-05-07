#include "helper.h"

void format_time(char *time_str) {
    // time
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(time_str, 20, "%d.%m.%Y %H:%M:%S", timeinfo);
}