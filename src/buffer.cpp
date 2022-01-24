//
// Created by xiamingjie on 2022/1/20.
//

#include "buffer.h"
#include <sys/uio.h>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>

Buffer::Buffer(size_t len) : buffer_(len), writePos_(0), readPos_(0) {}

size_t Buffer::readableBytes() const {
    return writePos_ - readPos_;
}

size_t Buffer::writeableBytes() const {
    return buffer_.size() - writePos_;
}

size_t Buffer::prependAbleBytes() const {
    return readPos_;
}

const char *Buffer::peek() const {
    return beginPtr_() + readPos_;
}

void Buffer::seek(size_t n) {
    if (n > readableBytes()) {
        return;
    }

    readPos_ += n;
}

char *Buffer::beginPtr_() {
    return &*buffer_.begin();
}

const char *Buffer::beginPtr_() const {
    return &*buffer_.begin();
}

void Buffer::makeSpace_(size_t len) {
    if (writeableBytes() + prependAbleBytes() < len) {
        buffer_.resize(writePos_ + len + 1);
    } else {
        const size_t readable = readableBytes();
        std::copy(beginPtr_() + readPos_, beginPtr_() + writePos_, beginPtr_());
        readPos_ = 0;
        writePos_ = readPos_ + readable;
    }
}

void Buffer::ensureWriteable(size_t len) {
    if (writeableBytes() < len) {
        makeSpace_(len);
    }
}

void Buffer::append(const char *data, size_t len) {
    if (data == nullptr || len <= 0) {
        return;
    }

    ensureWriteable(len);
    std::copy(data, data + len, beginPtr_() + writePos_);
}

void Buffer::append(const void *data, size_t len) {
    append(static_cast<const char *>(data), len);
}

void Buffer::clear() {
    buffer_.clear();
    buffer_.resize(BUFFER_SIZE);
    readPos_ = writePos_ = 0;
}

std::string Buffer::toString() {
    std::string str(peek(), readableBytes());
    clear();
    return str;
}

ssize_t Buffer::readFd(int fd, int *saveErrno) {
    char buff[65535];
    struct iovec iov[2];

    memset(buff, 0, sizeof(buff));
    const size_t writeable = writeableBytes();

    iov[0].iov_base = beginPtr_() + writePos_;
    iov[0].iov_len = writeable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    const ssize_t len = readv(fd, iov, 2);
    if (len < 0) {
        *saveErrno = errno;
    } else if (static_cast<size_t>(len) <= writeable) {
        writePos_ += len;
    } else {
        writePos_ = buffer_.size();
        append(buff, len - writeable);
    }

    return len;
}

ssize_t Buffer::writeFd(int fd, int *saveErrno) {
    size_t readSize = readableBytes();
    ssize_t len = send(fd, peek(), readSize, 0);
    if (len <= 0) {
        *saveErrno = errno;
        return len;
    }
    readPos_ += len;
    return len;
}