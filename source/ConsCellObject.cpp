#include "ConsCell.hpp"
#include "Error.hpp"
#include "alisp.hpp"
#include "Machine.hpp"
#include "SymbolObject.hpp"
#include "ConsCellObject.hpp"
#include "AtScopeExit.hpp"
#include "FArgs.hpp"
#include "Function.hpp"
#include <cstring>
#include <stdexcept>

namespace alisp
{

ALISP_INLINE ObjectPtr ConsCellObject::copy() const
{
    ListBuilder builder(*parent);
    cc->iterateList([&](Object* obj, bool circular, bool dotted) {
        if (circular) {
            throw exceptions::CircularList(toString());
        }
        else if (dotted) {
            throw exceptions::WrongTypeArgument(toString());
        }
        builder.append(obj->clone());
        return true;
    });
    return builder.get();
}
    
ALISP_INLINE ObjectPtr ConsCellObject::reverse() const
{
    std::unique_ptr<ConsCellObject> reversed = std::make_unique<ConsCellObject>(parent);
    cc->iterateList([&](Object* obj, bool circular, bool dotted) {
        if (circular) {
            throw exceptions::CircularList(toString());
        }
        else if (dotted) {
            throw exceptions::WrongTypeArgument(toString());
        }
        auto prev = reversed->cc;
        auto newcc = std::make_shared<ConsCell>();
        newcc->car = obj->clone();
        newcc->cdr =
            prev->car || prev->cdr ? std::make_unique<ConsCellObject>(prev, parent) : nullptr;
        reversed->cc = newcc;
        return true;
    });
    return reversed;
}

ALISP_INLINE std::shared_ptr<Function> ConsCellObject::resolveFunction() const
{
    if (isNil() || !car()->isSymbol()) {
        return nullptr;
    }
    auto& m = *parent;
    if (car()->asSymbol()->getSymbol() == m.getSymbol(m.parsedSymbolName("lambda"))) {
        auto func = std::make_shared<Function>();

        auto cc = this->cc->next();
        std::shared_ptr<ConsCellObject> closure =
            parent->makeConsCell(cc->car->clone(), cc->cdr->clone());
        const FuncParams fp = getFuncParams(*closure);
        func->minArgs = fp.min;
        func->maxArgs = fp.max;
        func->func = [&m, closure](FArgs& a) { return m.execute(*closure, a); };
        func->closure = closure;
        return func;
    }
    return nullptr;
}

ALISP_INLINE ObjectPtr ConsCellObject::eval()
{
    thread_local int depth = 0;
    depth++;
    const AtScopeExit onExit([]{ depth--; });
    if (depth >= 500) {
        throw exceptions::Error("Max recursion depth limit exceeded.");
    }
    
    auto &c = *cc;
    if (!c) {
        return std::make_unique<ConsCellObject>(parent);
    }
    const auto f = c.car->resolveFunction();
    if (f) {
        const int argc = countArgs(c.next());
        if (argc < f->minArgs || argc > f->maxArgs) {
            throw exceptions::WrongNumberOfArguments(argc);
        }
        FArgs args(*c.next(), *parent);
        args.funcName = f->name;
        return f->func(args);
    }
    if (c.car->isSymbol()) {
        throw exceptions::VoidFunction(c.car->toString());
    }
    else {
        throw exceptions::InvalidFunction(c.car->toString());
    }
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

ALISP_INLINE std::string ConsCellObject::toString(bool aesthetic) const
{
    if (!cc->car && !cc->cdr) {
        return NilName;
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
    const bool quote = carSym && carSym->name == parent->parsedSymbolName("quote");
    if (quote) {
        return "'" + (cc->next() ? cc->next()->car->toString() : std::string(""));
    }
    const bool fquote = carSym && carSym->name == parent->parsedSymbolName("function");
    if (fquote) {
        return "#'" + (cc->next() ? cc->next()->car->toString() : std::string(""));
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
    return p && p->car ? p->car->clone() : makeNil(parent);
}

ALISP_INLINE void ListBuilder::append(const Object& obj)
{
    append(obj.clone());
}

ALISP_INLINE void ListBuilder::append(std::unique_ptr<Object> obj)
{
    if (!m_list) {
        m_list = std::make_unique<ConsCellObject>(&m_parent);
    }
    if (!m_last) {
        m_list->cc = std::make_shared<ConsCell>();
        m_last = m_list->cc.get();
    }
    if (!m_last->car) {
        m_last->car = std::move(obj);
    }
    else {
        auto newCc = std::make_unique<ConsCellObject>(&m_parent);
        auto nextLast = newCc->cc.get();
        m_last->cdr = std::move(newCc);
        m_last = nextLast;
        m_last->car = std::move(obj);
    }
}

ALISP_INLINE std::unique_ptr<ConsCellObject> ListBuilder::get()
{
    if (!m_list) {
        return std::make_unique<ConsCellObject>(&m_parent);
    }
    auto r = std::move(m_list);
    m_list = nullptr;
    m_last = nullptr;
    return std::move(r);
}

const Symbol& ConsCellObject::convertTo(ConvertibleTo<const Symbol&>::Tag) const
{
    assert(isNil());
    return *parent->getSymbol(NilName);
}

Symbol& ConsCellObject::convertTo(ConvertibleTo<Symbol&>::Tag) const
{
    assert(isNil());
    return *parent->getSymbol(NilName);
}

const ConsCell& ConsCellObject::convertTo(ConvertibleTo<const ConsCell&>::Tag) const
{
    return *cc;
}

ConsCell& ConsCellObject::convertTo(ConvertibleTo<ConsCell&>::Tag) const
{
    return *cc;
}

bool ConsCellObject::canConvertTo(ConvertibleTo<const Symbol&>::Tag) const
{
    return isNil();
}

bool ConsCellObject::canConvertTo(ConvertibleTo<Symbol&>::Tag) const
{
    return isNil();
}

}
