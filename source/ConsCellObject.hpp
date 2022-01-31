#pragma once
#include "ConsCell.hpp"
#include "SharedDataObject.hpp"
#include "Sequence.hpp"
#include "Exception.hpp"

namespace alisp {

struct ConsCellObject : SharedDataObject, Sequence
{
    std::shared_ptr<ConsCell> cc;

    ConsCellObject() { cc = std::make_shared<ConsCell>(); }

    ConsCellObject(std::unique_ptr<Object> car, std::unique_ptr<Object> cdr) :
        ConsCellObject()
    {
        this->cc->car = std::move(car);
        if (!cdr || !(*cdr)) {
            return;
        }
        this->cc->cdr = std::move(cdr);
    }

    ConsCellObject(const ConsCellObject& o) : cc(o.cc) {}

    ~ConsCellObject()
    {
        if (cc) tryDestroySharedData();
    }        

    void reset() override { cc.reset(); }
    std::string toString() const override;
    bool isList() const override { return true; }
    bool isNil() const override { return !(*this); }
    bool operator!() const override { return !(*cc); }
    ConsCellObject* asList() override { return this; }
    const ConsCellObject* asList() const override { return this; }
    Object* car() const { return cc->car.get();  };
    Object* cdr() const { return cc->cdr.get();  };

    std::unique_ptr<Object> clone() const override
    {
        auto copy = std::make_unique<ConsCellObject>();
        copy->cc = cc;
        return copy;
    }

    bool equals(const Object &o) const override
    {
        const ConsCellObject *op = dynamic_cast<const ConsCellObject *>(&o);
        if (op && !(*this) && !(*op)) {
            return true;
        }
        return this->cc == op->cc;
    }

    size_t length() const override;
    std::unique_ptr<Object> eval() override;
    std::unique_ptr<Object> elt(std::int64_t index) const override;

    ConsCell::Iterator begin() const
    {
        return cc->begin();
    }
    
    ConsCell::Iterator end() const
    {
        return cc->end();
    }

    std::unique_ptr<ConsCellObject> deepCopy() const
    {
        std::unique_ptr<ConsCellObject> copy = std::make_unique<ConsCellObject>();
        ConsCell *origPtr = cc.get();
        ConsCell *copyPtr = copy->cc.get();
        assert(origPtr && copyPtr);
        while (origPtr) {
            if (origPtr->car) {
                if (origPtr->car->isList()) {
                    const auto list = dynamic_cast<ConsCellObject*>(origPtr->car.get());
                    copyPtr->car = list->deepCopy();
                }
                else {
                    copyPtr->car = origPtr->car->clone();
                }
            }
            auto cdr = origPtr->cdr.get();
            origPtr = origPtr->next();
            if (origPtr) {
                copyPtr->cdr = std::make_unique<ConsCellObject>();
                copyPtr = copyPtr->next();
            }
            else if (cdr) {
                assert(!cdr->isList());
                copyPtr->cdr = cdr->clone(); 
            }
        }
        return copy;
    }

    void traverse(const std::function<bool(const Object&)>& f) const override;

    const void* sharedDataPointer() const override { return cc.get(); }
    size_t sharedDataRefCount() const override { return cc.use_count(); }
};

inline std::unique_ptr<Object> makeNil()
{
    return std::make_unique<ConsCellObject>();
}

inline std::unique_ptr<ConsCellObject> makeList()
{
    return std::make_unique<ConsCellObject>();
}

class ListBuilder
{
    std::unique_ptr<ConsCellObject> m_list;
    ConsCell* m_last = nullptr;
public:
    void append(std::unique_ptr<Object> obj);
    std::unique_ptr<ConsCellObject> get();
};

}
