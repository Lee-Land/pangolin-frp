//
// Created by xiamingjie on 2022/1/19.
//

#ifndef PANGOLIN_FRP_PROTO_H
#define PANGOLIN_FRP_PROTO_H

#include <string>

class Host {
public:
    std::string hostName;
    int port;
};

/**
 * 工作类型: Server/Client
 * */
enum class RunType {
    SERVER,
    CLIENT
};

#endif //PANGOLIN_FRP_PROTO_H
