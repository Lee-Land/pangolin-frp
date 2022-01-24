//
// Created by xiamingjie on 2021/11/27.
//

#ifndef LOGGER_H
#define LOGGER_H

#include <cstdio>
#include <string>
#include <unordered_map>
#include <cstring>

/**
 * 日志等级
 * */
enum class Level {
    INFO = 0, DEBUG, WARN, ALERT, ERROR
};

class Logger final {
public:
    //    static Logger* getInstance();
    //    static void info(const char* content, ...);
    //    static void warring(const char* content, ...);
    //    static void error(const char* content, ...);
    //    static void debug(const char* content, ...);
public:
    static void write(Level level,
                      const char *file_name,
                      int line_num,
                      const char *format,
                      ...);

private:
    Logger() = default;

    ~Logger() = default;

private:
    //    std::string m_log_dir;  //日志目录
    //    std::queue<std::string> m_block_queue;    //阻塞队列
};

#define FILENAME(path) strrchr(path, '/') ? strrchr(path, '/') + 1 : path

#define LOG_INFO(format, ...) \
  Logger::write(Level::INFO, FILENAME(__FILE__), __LINE__, format, ##__VA_ARGS__)
#define LOG_WARN(format, ...) \
  Logger::write(Level::WARN, FILENAME(__FILE__), __LINE__, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) \
  Logger::write(Level::ERROR, FILENAME(__FILE__), __LINE__, format, ##__VA_ARGS__)
#define LOG_DEBUG(format, ...) \
  Logger::write(Level::DEBUG, FILENAME(__FILE__), __LINE__, format, ##__VA_ARGS__)
#define LOG_ALERT(format, ...) \
  Logger::write(Level::ALERT, FILENAME(__FILE__), __LINE__, format, ##__VA_ARGS__)
#define LOG_TEST(format, ...) \
  Logger::write(Level::DEBUG, FILENAME(__FILE__), __LINE__, format, ##__VA_ARGS__)

#endif  // LOGGER_H
