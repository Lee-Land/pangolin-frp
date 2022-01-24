//
// Created by xiamingjie on 2022/1/19.
//

#include "inielement.h"

namespace ini {
    using namespace std;

    Value::Value(const std::string &&value) {
        value_ = value;
    }

    Value & Value::operator=(const std::string &value) {
        value_ = value;
        return *this;
    }

    ostream & operator<<(std::ostream &os, const Value& value) {
        os << value.value_;
        return os;
    }

    int Value::asInt() {
        return stoi(value_);
    }

    double Value::asDouble() {
        return stod(value_);
    }

    string Value::asString() {
        return value_;
    }

    Parameters::Parameters() : iter_(begin()) {}

    Value & Parameters::operator[](const std::string &key) {
        return argsPair_[key];
    }

    Iterator Parameters::begin() {
        return argsPair_.begin();
    }

    Iterator Parameters::end() {
        return argsPair_.end();
    }

    void Parameters::operator++() {
        iter_++;
    }

    bool Parameters::operator!=(Iterator& iter) {
        return iter_ != iter;
    }

    ValuePair  Parameters::operator*(Iterator &iter) {
        return (*iter);
    }
}