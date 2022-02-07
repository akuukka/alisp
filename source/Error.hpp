#pragma once
#include "alisp.hpp"
#include <stdexcept>
#include <string>

namespace alisp {

class Machine;
struct Object;
struct ConsCellObject;
struct SymbolObject;

namespace exceptions {

struct Exception : std::runtime_error
{
    Exception(std::string msg) : std::runtime_error(std::move(msg)) {}
};

#define DEFINE_EXCEPTION(name)                                  \
    struct name : Exception {                                   \
        (name)(std::string msg) : Exception(msg) {}             \
    };                                                          \

DEFINE_EXCEPTION(UnableToEvaluate)
DEFINE_EXCEPTION(SyntaxError)

struct VoidVariable : Exception
{
    VoidVariable(std::string vname) : Exception("void-variable: " + vname) {}
};

struct WrongNumberOfArguments : Exception
{
    WrongNumberOfArguments(int num) :
        Exception("wrong-number-of-arguments: " + std::to_string(num))
    {

    }
};

struct Error : Exception
{
    std::unique_ptr<SymbolObject> sym;
    std::unique_ptr<Object> data;
    
    Error(std::unique_ptr<SymbolObject> sym,
          std::unique_ptr<Object> data);
    ~Error();
    
    Error(std::string msg,
          std::string symbolName = ConvertParsedNamesToUpperCase ? "ERROR" : "error");
    std::string symbolName, message;
    void onHandle(Machine& m);
    std::string getMessageString();
};

struct ArithError : Error
{
    ArithError(std::string msg) :
        Error(msg, ConvertParsedNamesToUpperCase ? "ARITH-ERROR" : "arith-error") {}
};

struct WrongTypeArgument : Error
{
    WrongTypeArgument(std::string msg) :
        Error(msg, ConvertParsedNamesToUpperCase ? "WRONG-TYPE-ARGUMENT" : "wrong-type-argument") {}
};

struct VoidFunction : Error
{
    VoidFunction(std::string msg) :
        Error(msg, ConvertParsedNamesToUpperCase ? "VOID-FUNCTION" : "void-function") {}
};

struct InvalidFunction : Error
{
    InvalidFunction(std::string msg) :
        Error(msg, ConvertParsedNamesToUpperCase ? "INVALID-FUNCTION" : "invalid-function") {}
};

struct CircularList : Error
{
    CircularList(std::string msg) :
        Error(msg, ConvertParsedNamesToUpperCase ? "CIRCULAR-LIST" : "circular-list") {}
};


} // namespace exceptions
}
