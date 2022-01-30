#include "Machine.hpp"

namespace alisp
{

void initSequenceFunctions(Machine& m)
{
    m.defun("length", [](const Sequence* ptr) {
        return ptr->length();
    });
    m.defun("sequencep", [](ObjectPtr obj) {
        return dynamic_cast<Sequence*>(obj.get()) != nullptr;
    });
}

}
