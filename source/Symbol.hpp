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
    std::string name;
    std::unique_ptr<Object> variable;
    std::unique_ptr<Function> function;
};

}
