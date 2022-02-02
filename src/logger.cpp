//
// Created by xiamingjie on 2021/11/27.
//
#include "logger.h"

#include <cstdarg>
#include <cstring>
#include <ctime>

#define LOG_BUFFER_SIZE 2048

using std::string;

bool Logger::debug_ = false;

void Logger::closeDebug() {
    debug_ = false;
}

void Logger::openDebug() {
    debug_ = true;
}

/**
 * 不同日志等级对应的标签
 * */
static const char* level2tag[] = {
        "[INFO]",
        "[DEBUG]",
        "[WARNING]",
        "[ALERT]",
        "[ERROR]"
};

void Logger::write(Level level,
                   const char *file_name,
                   int line_num,
                   const char *format,
                   ...) {
    if (!debug_ && level == Level::DEBUG) return;

    time_t tmp = time(nullptr);
    struct tm *cur_time = localtime(&tmp);
    if (!cur_time) {
        return;
    }

    char arg_buffer[LOG_BUFFER_SIZE];
    memset(arg_buffer, '\0', LOG_BUFFER_SIZE);

    strftime(arg_buffer, LOG_BUFFER_SIZE - 1, "[ %x %X ] ", cur_time);
    printf("%s", arg_buffer);
    printf("%s:%04d ", file_name, line_num);
    printf("%s ", level2tag[static_cast<int>(level)]);

    va_list arg_list;
    va_start(arg_list, format);
    memset(arg_buffer, '\0', LOG_BUFFER_SIZE);
    vsnprintf(arg_buffer, LOG_BUFFER_SIZE - 1, format, arg_list);
    printf("%s\n", arg_buffer);
    fflush(stdout);
    va_end(arg_list);
}