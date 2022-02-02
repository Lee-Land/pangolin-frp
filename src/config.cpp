//
// Created by xiamingjie on 2021/12/20.
//

#include "config.h"
#include "logger.h"

#include <unistd.h>
#include <cstring>
#include <fstream>

using std::string;
using std::ifstream;

const char* VERSION = "1.0";

void showLogo() {
    LOG_INFO("                                   .__  .__        ");
    LOG_INFO("___________    ____    ____   ____ |  | |__| ____  ");
    LOG_INFO("\\____ \\__  \\  /    \\  / ___\\ /  _ \\|  | |  |/    \\ ");
    LOG_INFO("|  |_> > __ \\|   |  \\/ /_/  >  <_> )  |_|  |   |  \\");
    LOG_INFO("|   __(____  /___|  /\\___  / \\____/|____/__|___|  /");
    LOG_INFO("|__|       \\/     \\//_____/                     \\/ ");
}

void usage(const char *name) {
    printf("usage: %s [-h] [-v] [-f config.json]\n", name);
}

string Config::parse(int argc, char **argv) {
    string fileName;
    int option;
    while ((option = getopt(argc, argv, "f:vh")) != -1) {
        switch (option) {
            case 'v': {  //版本信息
                printf("%s %s\n", basename(argv[0]), VERSION);
                exit(0);
            }
            case 'h': {  //帮助
                usage(basename(argv[0]));
                exit(0);
            }
            case 'f': {  //指定文件名
                fileName = string(optarg);
                break;
            }
            case '?': {
                LOG_ERROR("invalid option %c", option);
                usage(basename(argv[0]));
                exit(1);
            }
            default: {
                usage(basename(argv[0]));
                exit(0);
            }
        }
    }

    if (fileName.empty()) {
        LOG_ERROR("please specified the config file.");
        exit(1);
    }

    return fileName;
}