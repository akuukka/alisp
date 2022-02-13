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

ALISP_INLINE Object* ConsCellObject::cadr() const
{
    return cdr() && cdr()->isList() ? cdr()->asList()->car() : nullptr;
}

ALISP_INLINE const ConsCellObject* ConsCellObject::next() const
{
    return cdr() && cdr()->isList() ? cdr()->asList() : nullptr;
}

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

ALISP_INLINE ListPtr ConsCellObject::mapCar(const Function& func) const
{
    ListBuilder builder(*parent);
    if (!isNil()) {
        cc->iterateList([&](Object* obj, bool circular, Object* dot) {
            if (circular) {
                throw exceptions::CircularList(toString());
            }
            else if (dot) {
                throw exceptions::WrongTypeArgument(toString());
            }
            ConsCell cc;
            cc.car = parent->quote(obj->clone());
            FArgs args(cc, *parent);
            builder.append(func.func(args));
            return true;
        });
    }
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
            (prev && (prev->car || prev->cdr)) ?
            std::make_unique<ConsCellObject>(prev, parent) : nullptr;
        reversed->cc = newcc;
        return true;
    });
    return reversed;
}

ALISP_INLINE std::unique_ptr<ConsCellObject> ConsCellObject::deepCopy() const
{
    ListBuilder builder(*parent);
    if (cc) {
        cc->iterateList([&](Object* object, bool circular, Object* dotCdr) {
            if (circular) {
                throw std::runtime_error("Circular list");
            }
            if (object->isList()) {
                builder.append(object->asList()->deepCopy());
            }
            else {
                builder.append(object->clone());
            }
            if (dotCdr) {
                builder.dot(dotCdr->clone());
            }
            return true;
        });
    }
    return builder.get();
}

ObjectPtr expand(Machine& m,
                 ConsCellObject* code,
                 std::function<Object*()> paramSource);

ALISP_INLINE std::shared_ptr<Function> ConsCellObject::resolveFunction() const
{
    if (isNil()) {
        if (parent->getSymbol(NilName)->function) {
            return parent->getSymbol(NilName)->function->resolveFunction();
        }
        throw exceptions::VoidFunction(NilName);
    }
    if (!car()->isSymbol()) {
        return SharedValueObjectBase::resolveFunction();
    }
    auto& m = *parent;
    const bool macro = car()->asSymbol()->getSymbol() == m.getSymbol(MacroName);
    if (macro) {
        auto func = std::make_shared<Function>(*parent);
        auto cc = this->cc->next();
        std::shared_ptr<ConsCellObject> closure =
            parent->makeConsCell(cc->car->clone(), cc->cdr->clone());
        const FuncParams fp = getFuncParams(*closure->cdr()->asList());
        func->minArgs = fp.min;
        func->maxArgs = fp.max;
        func->func = [&m, closure](FArgs& a) {
            return expand(m, closure.get(), [&a](){ return a.pop(false); })->eval();
        };
        func->isMacro = true;
        return func;
    }
    const bool lambda = car()->asSymbol()->getSymbol() == m.getSymbol(LambdaName);
    if (lambda) {
        auto func = std::make_shared<Function>(*parent);
        auto cc = this->cc->next();
        std::shared_ptr<ConsCellObject> closure =
            parent->makeConsCell(cc->car->clone(), cc->cdr->clone());
        const FuncParams fp = getFuncParams(*closure);
        func->minArgs = fp.min;
        func->maxArgs = fp.max;
        func->func = [&m, closure](FArgs& a) { return m.execute(*closure, a); };
        return func;
    }
    return SharedValueObjectBase::resolveFunction();
}

ALISP_INLINE bool ConsCellObject::equals(const Object& o) const
{
    const ConsCellObject *op = dynamic_cast<const ConsCellObject*>(&o);
    if (op && !(*this) && !(*op)) { return true; }
    return op && cc == op->cc;
}

ALISP_STATIC int countArgs(const ConsCell* cc)
{
    if (!cc || !(*cc)) {
        return 0;
    }
    int i = 0;
    while (cc) {
        i++;
        cc = cc->next();
    }
    return i;
}

ALISP_INLINE ObjectPtr ConsCellObject::eval()
{
    thread_local int depth = 0;
    depth++;
    const AtScopeExit onExit([]{ depth--; });
    if (depth >= 500) {
        throw exceptions::Error("Max recursion depth limit exceeded.");
    }
    
    if (!cc || !(*cc)) {
        return std::make_unique<ConsCellObject>(parent);
    }
    try {
        auto &c = *cc;
        const auto f = car()->resolveFunction();
        assert(f && "Throws if fails");
        const int argc = countArgs(c.next());
        if (argc < f->minArgs || argc > f->maxArgs) {
            throw exceptions::WrongNumberOfArguments(argc);
        }
        FArgs args(*c.next(), *parent);
        return f->func(args);
    }
    catch (exceptions::Error& err) {
        err.stackTrace += toString() + "\n";
        throw;
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
    if (!cc || !cc->car && !cc->cdr) {
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
        if (car->isList() && !car->isNil()) {
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

ALISP_INLINE void ListBuilder::dot(std::unique_ptr<Object> obj)
{
    m_last->cdr = std::move(obj);
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
        auto newCc = std::make_unique<ConsCellObject>(std::move(obj), nullptr, &m_parent);
        auto nextLast = newCc->cc.get();
        m_last->cdr = std::move(newCc);
        m_last = nextLast;
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

bool ConsCellObject::canConvertTo(ConvertibleTo<ConsCell&>::Tag) const
{
    return !isNil();
}

bool ConsCellObject::canConvertTo(ConvertibleTo<const ConsCell&>::Tag) const
{
    return !isNil();
}

}
