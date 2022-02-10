#include "alisp.hpp"
#include "FArgs.hpp"

namespace alisp
{

ALISP_INLINE Object* FArgs::pop(bool eval)
{
    if (!cc) {
        return nullptr;
    }
    auto self = cc->car->trySelfEvaluate();
    if (self) {
        cc = cc->next();
        return self;
    }
    if (!eval) {
        auto ptr = cc->car.get();
        cc = cc->next();
        return ptr;
    }
    argStorage.push_back(cc->car->eval());
    cc = cc->next();
    return argStorage.back().get();
}

ALISP_INLINE ObjectPtr FArgs::evalAll(ConsCell* begin)
{
    auto code = begin ? begin : cc;
    while (code->next()) {
        code->car->eval();
        code = code->next();
    }
    return code->car->eval();
}

}
