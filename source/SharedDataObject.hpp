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

inline void SharedDataObject::tryDestroySharedData()
{
    if (!sharedDataPointer()) {
        return;
    }
    if (markedForCycleDeletion) {
        markedForCycleDeletion->erase(this);
        if (Object::destructionDebug()) {
            std::cout << this << " was deleted.\n";
        }
        return;
    }
    if (Object::destructionDebug()) {
        std::cout << "A reference to shared data " << toString()
                  << " is about to be reduced by one. this=" << this << std::endl;
    }

    // Traverse through the list. For every cons cell encountered, count how many times
    // it is referenced by the list.
    struct RefData {
        size_t refsFromCycle = 0;
        size_t totalRefs = 0; // except ref from this->cc as we are possibly about the del it!
    };
    std::map<const void*, RefData> referredTimes;
    size_t maxUseCount = 0;
    traverse([&](const Object& baseObj) {
        const SharedDataObject* obj = dynamic_cast<const SharedDataObject*>(&baseObj);
        if (!obj || !obj->sharedDataPointer()) {
            return false;
        }
        auto ptr = obj->sharedDataPointer();
        referredTimes[ptr].refsFromCycle++;
        referredTimes[ptr].totalRefs = obj->sharedDataRefCount();
        if (referredTimes[ptr].refsFromCycle >= 2) {
            return false;
        }
        return true;
    });
    for (auto& p : referredTimes) {
        if (Object::destructionDebug()) {
            std::cout << p.first << ": totalRefs=" << p.second.totalRefs
                      << ", refsFromCycle=" << p.second.refsFromCycle << std::endl;
        }
        if (p.second.totalRefs > p.second.refsFromCycle) {
            // Fair enough. Somebody is still referring to the cycle from outside it.
            return;
        }
    }

    if (Object::destructionDebug()) {
        std::cout << "The whole cycle is unreachable!\n";
    }
    std::set<SharedDataObject*> clearList;
    traverse([&](const Object& baseObj) {
        const SharedDataObject* obj = dynamic_cast<const SharedDataObject*>(&baseObj);
        if (!obj || !obj->sharedDataPointer()) {
            return false;
        }
        auto cc = const_cast<SharedDataObject*>(obj);
        if (clearList.count(cc)) {
            return false;
        }
        clearList.insert(cc);
        cc->markedForCycleDeletion = &clearList;
        return true;
    });
    while (clearList.size()) {
        if (Object::destructionDebug()) {
            std::cout << "Still " << clearList.size() << " objects left to destroy!\n";
            for (auto p : clearList) {
                std::cout << "   " << p << std::endl;
            }
        }
        auto p = *clearList.begin();
        clearList.erase(clearList.begin());
        if (Object::destructionDebug()) {
            std::cout << p << " about to be reseted\n";
        }
        p->reset();
        if (Object::destructionDebug())
            std::cout << p << " done...\n";
    }
    if (Object::destructionDebug())
        std::cout << "Done deleting circular list!\n";
}

}
