#pragma once
#include <map>
#include "Object.hpp"

namespace alisp
{

struct SharedValueObjectBase : Object
{
    std::set<SharedValueObjectBase*>* markedForCycleDeletion = nullptr;
    void tryDestroySharedData(); // Derived classes must call this from destructor.
    virtual const void* sharedDataPointer() const = 0;
    virtual size_t sharedDataRefCount() const = 0;
    virtual void reset() = 0;
};

template<typename T>
struct SharedValueObject : SharedValueObjectBase
{
    std::shared_ptr<T> value;

    SharedValueObject(std::shared_ptr<T> value) : value(value) {}
    ~SharedValueObject() { tryDestroySharedData(); }
    const void* sharedDataPointer() const override { return value.get(); }
    size_t sharedDataRefCount() const override { return value.use_count(); };
    void reset() override { value.reset(); }

    bool equals(const Object& o) const override
    {
        const SharedValueObject* op = dynamic_cast<const SharedValueObject*>(&o);
        if (!op) {
            return false;
        }
        return value == op->value;
    }
};

}
