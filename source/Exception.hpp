#include <stdexcept>
#include <string>

namespace alisp {
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
DEFINE_EXCEPTION(Error)
DEFINE_EXCEPTION(SyntaxError)
DEFINE_EXCEPTION(ArithError)

struct VoidFunction : Exception
{
    VoidFunction(std::string fname) : Exception("void-function: " + fname) {}
};

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

struct WrongTypeArgument : Exception
{
    WrongTypeArgument(std::string arg) :
        Exception("wrong-type-argument: " + arg)
    {

    }
};

} // namespace exceptions
}
