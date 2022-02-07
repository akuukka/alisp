#pragma once
#include <memory>
#include <string>

namespace alisp
{

struct Function;
struct Object;
struct ConsCell;
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

Object* get(const ConsCell& plist, const Object& property);
Object* get(const ConsCellObject& plist, const Object& property);
Object* get(std::unique_ptr<ConsCellObject>& plist, const Object& property);

}
