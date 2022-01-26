#include <functional>

namespace alisp
{

struct AtScopeExit
{
    std::function<void()> f;
    
    AtScopeExit(std::function<void()> f) : f(std::move(f))
    {

    }
    
    ~AtScopeExit()
    {
        f();
    }
};

}
