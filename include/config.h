//
// Created by xiamingjie on 2021/12/20.
//

#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>

void showLogo();
void usage(const char* name);

class Config {
public:
    /**
     * 命令行参数解析出文件名
     * */
    void parse(int argc, char* argv[]);

public:
    std::string fileName;
    bool isDebug;
};

#endif //CONFIG_H
