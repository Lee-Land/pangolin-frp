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
        Host internal, external;
        //遍历配置项
        for (int i = 0; i < size; ++i) {
            parameter = iniConfig[i];
            string internal_str = parameter["internal"].asString();

            string::size_type n = internal_str.find(':');
            //格式错误
            if (n == string::npos) {
                throw ini::ErrorLog("parameter of internal invalid.");
            }

            internal.hostName = internal_str.substr(0, n);
            internal.port = stoi(internal_str.substr(n + 1, internal_str.length()));

            string external_str = parameter["external"].asString();
            n = external_str.find(':');
            if (n == string::npos) {
                throw ini::ErrorLog("parameter of external invalid.");
            }

            external.hostName = external_str.substr(0, n);
            external.port = stoi(external_str.substr(n + 1, external_str.length()));

            //将映射对添加进映射数组
            mappings.emplace_back(pair<Host, Host>(internal, external));
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