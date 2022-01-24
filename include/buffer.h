//
// Created by xiamingjie on 2021/12/15.
//

#ifndef BUFFER_H
#define BUFFER_H

#include <cstdint>
#include <vector>
#include <string>

std::string generateToken();

/**
 * 缓冲区读写状态码
 * */
enum class RET_CODE {
    OK,
    NOTHING,
    IO_ERR,
    CLOSED,
    BUFFER_FULL,
    BUFFER_EMPTY,
    TRY_AGAIN
};

const static size_t BUFFER_SIZE = 2048;

/**
 * 循环缓冲区
 * */
class Buffer {
public:
    explicit Buffer(size_t len = BUFFER_SIZE);
    ~Buffer() = default;

    size_t readableBytes() const;
    size_t writeableBytes() const;
    size_t prependAbleBytes() const;

    const char* peek() const;
    void seek(size_t n);

    void append(const void* data, size_t len);
    void append(const char* data, size_t len);

    void ensureWriteable(size_t len);

    ssize_t readFd(int fd, int* saveErrno);    //将接收缓冲区内所有数据读取完
    ssize_t writeFd(int fd, int* saveErrno);
    void clear();
    std::string toString();

private:
    void makeSpace_(size_t len);
    char* beginPtr_();
    const char* beginPtr_() const;

private:
    std::vector<char> buffer_;
    std::size_t readPos_;
    std::size_t writePos_;
};

#endif //BUFFER_H
