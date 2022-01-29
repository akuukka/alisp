#include "alisp.hpp"

namespace alisp
{

const ConsCell* ConsCell::next() const
{
    auto cc = dynamic_cast<ConsCellObject *>(this->cdr.get());
    if (cc) {
        return cc->cc.get();
    }
    return nullptr;
}

ConsCell* ConsCell::next()
{
    auto cc = dynamic_cast<ConsCellObject *>(this->cdr.get());
    if (cc) {
        return cc->cc.get();
    }
    return nullptr;
}

void ConsCell::traverse(const std::function<bool(const ConsCell*)>& f) const
{
    if (!*this) {
        return;
    }
    const ConsCell* cell = this;
    while (cell) {
        if (!f(cell)) {
            return;
        }
        if (cell->car->isList()) {
            cell->car->asList()->cc.get()->traverse(f);
        }
        cell = cell->next();
    }
}

bool ConsCell::isCyclical() const
{
    if (!*this) {
        return false;
    }
    size_t selfTimes = 0;
    bool cycled = false;
    std::set<const ConsCell*> visited;
    traverse([&](const ConsCell* cell) {
        if (cell == this) {
            selfTimes++;
            if (selfTimes >= 2) {
                cycled = true;
                return false;
            }
        }
        if (visited.count(cell)) {
            return false;
        }
        visited.insert(cell);
        return true;
    });
    return cycled;
}

}
