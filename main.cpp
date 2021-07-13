#include <iostream>
#include "JSON.h"


const char* raw = R"(
[
  1,
  2,
  true,
  1.4,
  {
    "key": 1,
    "key2": 3,
    "key3": [1, false, "lol"]
  },
  null,
  null,
  {
    "key": null,
    "key3": [1, 2, "lol"]
  }
]
)";


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
    std::cout << test.dump<4>() << std::endl;
    std::cout << test.dump<2>() << std::endl;
    std::cout << test.dump<0>() << std::endl;

    // std::cout << JSON::JSON::load(raw).dump<2>();
    return 0;
}
