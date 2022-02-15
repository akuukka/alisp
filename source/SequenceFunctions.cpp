#include "Error.hpp"
#include "Machine.hpp"
#include <cstdint>
#include <stdexcept>
#include "Sequence.hpp"
#include "ConsCellObject.hpp"

namespace alisp
{

void Machine::initSequenceFunctions()
{
    defun("length", [](const Sequence& ptr) {
        return ptr.length();
    });
    defun("elt", [](const Sequence& ptr, std::int64_t index) {
        try {
            return ptr.elt(index);
        }
        catch (std::runtime_error& ex) {
            throw exceptions::Error("Index out of range.");
        }
    });
    defun("sequencep", [](ObjectPtr obj) {
        return dynamic_cast<Sequence*>(obj.get()) != nullptr;
    });
    defun("reverse", [](const Sequence& seq) { return seq.reverse(); });
    defun("copy-sequence", [](const Sequence& seq) { return seq.copy(); });
    defun("mapcar", [](const Function& func, const Sequence& seq) { return seq.mapCar(func); });
    defun("mapc", [](const Function& func, const Object& obj) {
        auto seq = dynamic_cast<const Sequence*>(&obj);
        if (!seq) {
            throw exceptions::WrongTypeArgument(obj.toString());
        }
        seq->mapCar(func);
        return obj.clone();
    });
    defun("nreverse", [this](const Object& obj) -> ObjectPtr {
        if (obj.isList()) {
            auto list = obj.asList();
            auto head = list->cc;
            auto tail = list->cc;
            std::set<const ConsCell*> heads;
            while (tail && tail->cdr) {
                assert(tail->cdr->isList());
                auto newhead = tail->cdr->asList()->cc;
                if (heads.count(newhead.get())) {
                    throw exceptions::CircularList(obj.toString());
                }
                heads.insert(newhead.get());
                assert(tail->cdr->asList()->cc.get());
                std::shared_ptr<ConsCell> oldc;
                if (newhead->cdr) {
                    if (!newhead->cdr->asList()) {
                        auto to = ConsCellObject(newhead, this);
                        throw exceptions::WrongTypeArgument(to.toString());
                    }
                    assert(newhead->cdr->asList());
                    assert(newhead->cdr->asList()->cc);
                    oldc = newhead->cdr->asList()->cc;
                }
                newhead->cdr = std::make_unique<ConsCellObject>(head, this);
                tail->cdr = oldc ? std::make_unique<ConsCellObject>(oldc, this) : nullptr;
                head = newhead;
            }
            return std::make_unique<ConsCellObject>(head, this);
        }
        else {
            throw exceptions::WrongTypeArgument(obj.toString());
        }
    });
}

}
