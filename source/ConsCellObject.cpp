#include "alisp.hpp"

namespace alisp
{

std::unique_ptr<Object> ConsCellObject::eval()
{
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

void ConsCellObject::traverse(const std::function<bool(const Object&)>& f) const
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

std::string ConsCellObject::toString() const
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

}
