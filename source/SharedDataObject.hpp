#pragma once
#include <map>
#include "Object.hpp"

namespace alisp
{

struct SharedDataObject : Object
{
    std::set<SharedDataObject*>* markedForCycleDeletion = nullptr;
    void tryDestroySharedData(); // Derived classes must call this from destructor.
    virtual const void* sharedDataPointer() const = 0;
    virtual size_t sharedDataRefCount() const = 0;
    virtual void reset() = 0;
};

}
