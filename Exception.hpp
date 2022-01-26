#include <stdexcept>
#include <string>

namespace alisp {
namespace exceptions {

#define DEFINE_EXCEPTION(name)                                  \
    struct name : std::runtime_error {                          \
        (name)(std::string msg) : std::runtime_error(msg) {}    \
    };                                                          \

DEFINE_EXCEPTION(UnableToEvaluate)
DEFINE_EXCEPTION(Error)
DEFINE_EXCEPTION(SyntaxError)
DEFINE_EXCEPTION(ArithError)

struct VoidFunction : std::runtime_error
{
    VoidFunction(std::string fname) : std::runtime_error("void-function: " + fname) {}
};

struct VoidVariable : std::runtime_error
{
    VoidVariable(std::string vname) : std::runtime_error("void-variable: " + vname) {}
};

struct WrongNumberOfArguments : std::runtime_error
{
    WrongNumberOfArguments(int num) :
        std::runtime_error("wrong-number-of-arguments: " + std::to_string(num))
    {

    }
};

struct WrongTypeArgument : std::runtime_error
{
    WrongTypeArgument(std::string arg) :
        std::runtime_error("wrong-type-argument: " + arg)
    {

    }
};

} // namespace exceptions
}
