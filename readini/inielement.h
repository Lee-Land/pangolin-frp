//
// Created by xiamingjie on 2022/1/19.
//

#ifndef READINI_INIELEMENT_H
#define READINI_INIELEMENT_H

#include <unordered_map>
#include <string>

namespace ini {

    class Value {
    public:
        Value() = default;
        explicit Value(const std::string&& value);
        Value & operator=(const std::string& value);

        std::string asString();
        int asInt();
        double asDouble();

        friend std::ostream & operator << (std::ostream & os, const Value& value);

    private:
        std::string value_;
    };

    using ArgsPair = std::unordered_map<std::string, Value>;
    using Iterator = ArgsPair::iterator;
    using ValuePair = std::pair<std::string, Value>;

    class Parameters {
    public:
        Parameters();

        Value & operator[](const std::string& key);
        Iterator begin();
        Iterator end();
        void operator++();
        bool operator!=(Iterator& iter);
        ValuePair operator*(Iterator& iter);


    private:
        ArgsPair argsPair_;
        Iterator iter_;
    };
}


#endif //READINI_INIELEMENT_H
