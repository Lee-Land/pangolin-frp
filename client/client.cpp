//
// Created by xiamingjie on 2022/1/19.
//

#include <iostream>

#include "iniconfig.h"
#include "logger.h"
#include "config.h"
#include "proto.h"
#include "processpool.h"

using std::string;
using std::fstream;
using std::ios;
using std::ios_base;
using std::vector;
using std::pair;

int main(int argc, char* argv[]) {
    string configFileName = Config::parseFileName(argc, argv);

    fstream input(configFileName, ios::in);

    ini::IniConfig iniConfig;

    //映射对数组
    vector<pair<Host, Host>> mappings;

    try {
        //解析
        iniConfig.parse(input);

        size_t size = iniConfig.size();

        ini::Parameters parameter;
        Host intranet, publicc;
        //遍历配置项
        for (int i = 0; i < size; ++i) {
            parameter = iniConfig[i];
            string intranet_str = parameter["intranet"].asString();

            string::size_type n = intranet_str.find(':');
            //格式错误
            if (n == string::npos) {
                throw ini::ErrorLog("parameter of intranet invalid.");
            }

            intranet.hostName = intranet_str.substr(0, n);
            intranet.port = stoi(intranet_str.substr(n + 1, intranet_str.length()));

            string publicc_str = parameter["public"].asString();
            n = publicc_str.find(':');
            if (n == string::npos) {
                throw ini::ErrorLog("parameter of public invalid.");
            }

            publicc.hostName = publicc_str.substr(0, n);
            publicc.port = stoi(publicc_str.substr(n + 1, publicc_str.length()));

            //将映射对添加进映射数组
            mappings.emplace_back(pair<Host, Host>(intranet, publicc));
        }

    } catch (const ios_base::failure& e1) {
        LOG_ERROR("%s", e1.what());
        return 1;
    } catch (const ini::ErrorLog& e2) {
        LOG_ERROR("%s", e2.what());
        return 1;
    }

    showLogo();

    client::ProcessPool* pool = client::ProcessPool::create(mappings.size());
    if (pool != nullptr) {
        pool->run(mappings);
        delete pool;
    }

    return 0;
}