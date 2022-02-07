#pragma once
#include <memory>
#include <functional>

namespace alisp
{

struct Object;

struct ConsCell
{
    std::unique_ptr<Object> car;
    std::unique_ptr<Object> cdr;

    const ConsCell* next() const;
    ConsCell* next();
    bool operator!() const { return !car; }

    struct Iterator
    {
        ConsCell* ptr;
        bool operator!=(const Iterator& rhs) const { return ptr != rhs.ptr; }
        Iterator& operator++()
        {
            ptr = ptr->next();
            return *this;
        }

        Object& operator*() {
            return *ptr->car.get();
        }
    };
    
    struct ConstIterator
    {
        const ConsCell* ptr;
        bool operator!=(const ConstIterator& rhs) const { return ptr != rhs.ptr; }
        bool operator==(const ConstIterator& rhs) const { return ptr == rhs.ptr; }
        ConstIterator& operator++()
        {
            ptr = ptr->next();
            return *this;
        }

        ConstIterator operator+(int i) const
        {
            ConstIterator copy = *this;
            copy.ptr = copy.ptr->next();
            return copy;
        }

        Object& operator*() {
            return *ptr->car.get();
        }
    };

    Iterator begin() { return !*this ? end() : Iterator{this}; }
    Iterator end() { return Iterator{nullptr}; }
    ConstIterator begin() const { return !*this ? end() : ConstIterator{this}; }
    ConstIterator end() const { return ConstIterator{nullptr}; }

    void traverse(const std::function<bool(const ConsCell*)>& f) const;
    bool isCyclical() const;
};

}
