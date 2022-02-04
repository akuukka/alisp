#pragma once
#include <vector>
#include <string>

namespace alisp {

struct ConsCellObject;

struct FuncParams {
    int min = 0;
    int max = 0;
    bool rest = false;
    std::vector<std::string> names;
};

FuncParams getFuncParams(const ConsCellObject& closure);

}
