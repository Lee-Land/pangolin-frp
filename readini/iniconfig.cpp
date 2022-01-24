//
// Created by xiamingjie on 2022/1/18.
//

#include "iniconfig.h"

#include <cstdarg>
#include <cstring>

namespace ini {

    using namespace std;

    ErrorLog::ErrorLog(const char *format, ...) : message_() {
        va_list vaList;
        va_start(vaList, format);
        memset(message_, '\0', sizeof(message_));
        vsnprintf(message_, sizeof(message_) - 1, format, vaList);
        va_end(vaList);
    }

    ErrorLog::ErrorLog(const std::string &message, ...) : ErrorLog(message.c_str()) {}

    const char *ErrorLog::what() const noexcept {
        return message_;
    }

    IniConfig::IniConfig() : state_(IniState::NOTHING) {}

    std::string &trim(std::string &s) {
        if (!s.empty()) {
            s.erase(0, s.find_first_not_of(' '));
            s.erase(s.find_last_not_of(' ') + 1);
        }
        return s;
    }

    Parameters & IniConfig::operator[](const string& section_key) {
        return sections_[section_key];
    }

    Parameters & IniConfig::operator[](int index) {

        if (index >= sections_.size() || index < 0) {
            throw out_of_range("Index out of range.");
        }

        auto iter = sections_.begin();
        while (iter != sections_.end() && index-- > 0) {
            iter++;
        }

        return iter->second;
    }

    size_t IniConfig::size() const {
        return sections_.size();
    }

    void IniConfig::parse(istream &input) {
        if (input.fail()) {
            throw ios_base::failure("Failed to read configuration");
        }

        string last_section;
        int number = 0;
        for (string line; getline(input, line);) {  //按行读取
            number++;
            if (line.empty()) {   //空行
                continue;
            }

            line = trim(line);  //去除首尾空格

            int index = 0;
            size_t line_len = line.length();
            state_ = IniState::NOTHING;
            //有限状态机
            while (state_ != IniState::SUCCESS) {
                switch (state_) {
                    case IniState::NOTHING: {
                        for (; index < line_len; index++) {
                            if (line[index] == ';') {
                                state_ = IniState::COMMENTS;
                                break;
                            } else if (line[index] == '[') {
                                state_ = IniState::SECTION;
                                break;
                            } else if (line[index] == ']') {    //']'比'['先出现
                                state_ = IniState::SECTION_ERROR;
                                break;
                            } else if (line[index] != ' ') {
                                state_ = IniState::PARAMETERS;
                                break;
                            }
                        }
                        break;
                    }
                    case IniState::SECTION: {
                        string content = line.substr(index, line_len);
                        string::size_type comments_index = content.find(';');
                        string::size_type split_index = content.find(']');

                        //';'注释比']'先出现
                        if (comments_index <= split_index) {
                            state_ = IniState::SECTION_ERROR;
                            break;
                        }

                        if (split_index == string::npos) {    //'['没有闭合
                            state_ = IniState::SECTION_ERROR;
                            break;
                        }

                        last_section = content.substr(index + 1, split_index - 1);

                        state_ = IniState::SUCCESS;
                        break;
                    }
                    case IniState::PARAMETERS: {
                        string::size_type comments_index = line.find(';');
                        string::size_type split_index = line.find('=');

                        //';'注释号出现在'='号前面
                        if (comments_index <= split_index) {
                            state_ = IniState::PARAMETERS_ERROR;
                            break;
                        }

                        //没有'='号或者'='号处于开头或结尾
                        if (split_index == string::npos || split_index == index || split_index == line_len - 1) {
                            state_ = IniState::PARAMETERS_ERROR;
                            break;
                        }

                        string key = line.substr(index, split_index);
                        string value = line.substr(split_index + 1, line_len - 1);

                        comments_index = value.find(';');
                        //将注释部分抛弃
                        if (comments_index != string::npos) {
                            value = value.substr(0, comments_index);
                        }

                        sections_[last_section][trim(key)] = trim(value);

                        state_ = IniState::SUCCESS;

                        break;
                    }
                    case IniState::SECTION_ERROR: {
                        throw ErrorLog("[Section] format invalid in the file %d line.", number);
                    }
                    case IniState::PARAMETERS_ERROR: {
                        throw ErrorLog("Parameters format invalid int the file %d line.", number);
                    }
                    case IniState::COMMENTS:
                    case IniState::SUCCESS: {
                        state_ = IniState::SUCCESS;
                        break;
                    }
                }
            }
        }
    }
}