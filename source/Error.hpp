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

DEFINE_EXCEPTION(SyntaxError)

struct Error : Exception
{
    std::unique_ptr<SymbolObject> sym;
    std::unique_ptr<Object> data;
    std::string stackTrace;
    
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

struct SettingConstant : Error
{
    SettingConstant(std::string msg) :
        Error(msg, ConvertParsedNamesToUpperCase ? "SETTING-CONSTANT" : "setting-constant") {}
};

struct WrongNumberOfArguments : Error
{
    WrongNumberOfArguments(int num) :
        Error(std::to_string(num),
              ConvertParsedNamesToUpperCase ?
              "WRONG-NUMBER-OF-ARGUMENTS" : "wrong-number-of-arguments")
    {

    }
};

struct VoidVariable : Error
{
    VoidVariable(std::string vname) : Error(vname, 
                                            ConvertParsedNamesToUpperCase ?
                                            "VOID-VARIABLE" : "void-variable") {}
};


} // namespace exceptions
}
