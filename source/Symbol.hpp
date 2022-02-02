#pragma once
#include <memory>
#include <string>

namespace alisp
{

struct Function;
struct Object;
class Machine;

struct Symbol
{
    Machine* parent = nullptr;
    bool constant = false;
    bool local = false;
    std::string name;
    std::string description;
    std::unique_ptr<Object> variable;
    std::shared_ptr<Function> function;
};

}
