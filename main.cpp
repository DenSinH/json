#include <iostream>
#include "JSON.h"

#include "testing/raw_string.h"

int main() {
    JSON::JSON test = JSON::Array[{
        1,
        2,
        "hello",
        1.4,
        JSON::Object{
            {"key", 1},
            {"key2", 3},
            {"key3", {
                1, false, "lol"
            }}
        },
        nullptr,
        JSON::null,
        JSON::Object{
            {"key", nullptr},
            {"key3", {
                1, 2, "lol"
            }}
        }
    }];

    std::cout << test[4]["key3"][0].get<int>() << std::endl;

    JSON::JSON test2 = JSON::Object{
        {"key", 1},
        {"key2", 3},
        {"key3", {
            1, false, "lol"
        }}
    };

    std::cout << test2["key3"][2].get<std::string>() << std::endl;
    std::cout << test2.dump<2>() << std::endl;
    test2["key3"] = nullptr;
    test2["hello"] = JSON::Array[{
        1, 2, true, false, nullptr
    }];
    std::cout << test2.dump<2>() << std::endl;

    JSON::JSON::load(raw).dump_to<2>("./testing/out.json");
    std::cout << sizeof(JSON::JSON);  // 48

    return 0;
}
