#pragma once
#include "ConsCellObject.hpp"
#include <memory>
#include <string>

namespace alisp
{

struct Function;
struct Object;
struct ConsCellObject;
class Machine;

struct Symbol
{
    Machine* parent = nullptr;
    bool constant = false;
    bool local = false;
    std::string name;
    std::string description;
    std::unique_ptr<Object> variable;
    std::unique_ptr<ConsCellObject> plist;
    std::shared_ptr<Function> function;
};

}
