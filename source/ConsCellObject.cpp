#include "alisp.hpp"
#include "SymbolObject.hpp"
#include "ConsCellObject.hpp"
#include "AtScopeExit.hpp"
#include "FArgs.hpp"
#include <cstring>

namespace alisp
{

ALISP_INLINE std::unique_ptr<Object> ConsCellObject::eval()
{
    thread_local int depth = 0;
    depth++;
    const AtScopeExit onExit([]{ depth--; });
    if (depth >= 500) {
        throw exceptions::Error("Max recursion depth limit exceeded.");
    }
    
    auto &c = *cc;
    if (!c) {
        return std::make_unique<ConsCellObject>();
    }
    const Function* f = c.car->resolveFunction();
    if (f) {
        auto &m = *dynamic_cast<const SymbolObject*>(c.car.get())->parent;
        const int argc = countArgs(c.next());
        if (argc < f->minArgs || argc > f->maxArgs) {
            throw exceptions::WrongNumberOfArguments(argc);
        }
        FArgs args(*c.next(), m);
        return f->func(args);
    }
    throw exceptions::VoidFunction(c.car->toString());
}

ALISP_INLINE void ConsCellObject::traverse(const std::function<bool(const Object&)>& f) const
{
    if (!*this) {
        return;
    }
    const ConsCellObject* cell = this;
    while (cell) {
        if (!f(*cell)) {
            return;
        }
        cell->cc->car->traverse(f);
        cell = cell->cc->cdr ? cell->cc->cdr->asList() : nullptr;
    }    
}

ALISP_INLINE std::string ConsCellObject::toString() const
{
    if (!cc->car && !cc->cdr) {
        return "nil";
    }

    std::vector<const ConsCell*> cellPtrs;
    auto p = cc.get();
    bool infinite = false;
    while (p) {
        auto it = std::find(cellPtrs.begin(), cellPtrs.end(), p);
        if (it != cellPtrs.end()) {
            infinite = true;
            break;
        }
        cellPtrs.push_back(p);
        p = p->next();
    }

    const SymbolObject* carSym = dynamic_cast<const SymbolObject*>(car());
    const bool quote = carSym && carSym->name == "quote";
    if (quote) {
        return "'" + (cc->next() ? cc->next()->car->toString() : std::string(""));
    }

    auto carToString = [&](const Object* car) -> std::string {
        if (car->isList()) {
            const ConsCell* car2 = car->asList()->cc.get();
            if (car2->isCyclical()) {
                std::int64_t i = -1;
                car2->traverse([&](const ConsCell* cell) {
                    auto it = std::find(cellPtrs.begin(), cellPtrs.end(), car2);
                    if (it != cellPtrs.end()) {
                        i = std::distance(cellPtrs.begin(), it);
                        return false;
                    }
                    return true;
                });
                return "#" + std::to_string(i);
            }
        }
        return car->toString();
    };
        
    std::string s = "(";
    const ConsCell *t = cc.get();
    std::map<const ConsCell*, size_t> visited;
    std::int64_t index = 0;
    std::optional<std::int64_t> loopback;
    while (t) {
        if (!t->next() && t->cdr) {
            s += t->car->toString() + " . " + t->cdr->toString();
            break;
        }
        else if (infinite && visited[t->next()] == (cellPtrs.size() > 1 ? 2 : 1)) {
            s += ". ";
            if (cellPtrs.size() == 1) {
                loopback = 0;
            }
            s += "#" + std::to_string(*loopback);
            break;
        }
        if (infinite && !loopback && visited[t->next()]) {
            loopback = index;
        }
        visited[t]++;
        index++;
        s += t->car ? carToString(t->car.get()) : "";
        t = t->next();
        if (t) {
            s += " ";
        }
    }
    s += ")";
    return s;
}

template <>
ALISP_INLINE std::optional<ConsCellObject> getValue(const Object& sym)
{
    auto s = dynamic_cast<const ConsCellObject*>(&sym);
    if (s) {
        return *s;
    }
    return std::nullopt;
}

ALISP_INLINE size_t ConsCellObject::length() const
{
    if (!*this) {
        return 0;
    }
    std::set<const ConsCell*> visited;
    size_t l = 1;
    auto p = cc.get();
    visited.insert(p);
    while ((p = p->next())) {
        if (visited.count(p)) {
            throw exceptions::Error("Cyclical list length");
        }
        visited.insert(p);
        l++;
    }
    return l;
}

std::unique_ptr<Object> ConsCellObject::elt(std::int64_t index) const
{
    auto p = cc.get();
    for (std::int64_t i=0;i<index;i++) {
        p = p->next();
        if (!p) {
            break;
        }
    }
    return p && p->car ? p->car->clone() : makeNil();
}

ALISP_INLINE void ListBuilder::append(std::unique_ptr<Object> obj)
{
    if (!m_list) {
        m_list = std::make_unique<ConsCellObject>();
    }
    if (!m_last) {
        m_list->cc = std::make_shared<ConsCell>();
        m_last = m_list->cc.get();
    }
    if (!m_last->car) {
        m_last->car = std::move(obj);
    }
    else {
        auto newCc = std::make_unique<ConsCellObject>();
        auto nextLast = newCc->cc.get();
        m_last->cdr = std::move(newCc);
        m_last = nextLast;
        m_last->car = std::move(obj);
    }
}

ALISP_INLINE std::unique_ptr<ConsCellObject> ListBuilder::get()
{
    if (!m_list) {
        return std::make_unique<ConsCellObject>();
    }
    auto r = std::move(m_list);
    m_list = nullptr;
    m_last = nullptr;
    return std::move(r);
}

}
