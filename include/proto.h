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
 * 消息头部
 * */
#pragma pack(push, 1)
struct MsgHeader {
    uint8_t cmd;        //控制码 0:普通数据, 1:外网客户端发起连接
    uint32_t length;    //数据正文长度,不包含头部
};
#pragma pack(pop)

#endif //PANGOLIN_FRP_PROTO_H
