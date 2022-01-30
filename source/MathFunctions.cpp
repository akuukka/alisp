#include "ConsCellObject.hpp"
#include "Machine.hpp"
#include "ValueObject.hpp"
#include "alisp.hpp"

namespace alisp
{

ALISP_INLINE std::unique_ptr<Object> truncate(std::variant<double, std::int64_t> obj)
{
    try {
        const std::int64_t i = std::get<std::int64_t>(obj);
        return makeInt(i);
    }
    catch (std::bad_variant_access&) {
        return makeInt(static_cast<std::int64_t>(std::get<double>(obj)));
    }
}

void initMathFunctions(Machine& m)
{
    m.defun("truncate", truncate);
    m.defun("%", [](std::int64_t in1, std::int64_t in2) { return in1 % in2; });
    m.makeFunc("=", 1, 0xffff, [&m](FArgs& args) {
        std::int64_t i = 0;
        double f = 0;
        bool fp = false;
        bool first = true;
        for (auto sym : args) {
            if (sym->isFloat()) {
                const double v = sym->value<double>();
                if (!first) {
                    if (v != f) {
                        return makeNil();
                    }
                }
                f = v;
                fp = true;
            }
            else if (sym->isInt()) {
                const std::int64_t v = sym->value<std::int64_t>();
                if (!first) {
                    if ((!fp && v != i) || (fp && v != f)) {
                        return makeNil();
                    }
                }
                i = v;
                f = v;
            }
            else {
                throw exceptions::WrongTypeArgument(sym->toString());
            }
            first = false;
        }
        return m.makeTrue();
    });
    m.defun("1+", [](std::variant<double, std::int64_t> obj) -> std::unique_ptr<Object> {
        try {
            const std::int64_t i = std::get<std::int64_t>(obj);
            return makeInt(i+1);
        }
        catch (std::bad_variant_access&) {
            const double f = std::get<double>(obj);
            return makeFloat(f+1.0);
        }
    });
    m.makeFunc("+", 0, 0xffff, [](FArgs& args) {
        std::int64_t i = 0;
        double f = 0;
        bool fp = false;
        for (auto sym : args) {
            if (sym->isFloat()) {
                const double v = sym->value<double>();
                fp = true;
                i += v;
                f += v;
            }
            else if (sym->isInt()) {
                const std::int64_t v = sym->value<std::int64_t>();
                i += v;
                f += v;
            }
            else {
                throw exceptions::WrongTypeArgument(sym->toString());
            }
        }
        return fp ? static_cast<std::unique_ptr<Object>>(makeFloat(f))
            : static_cast<std::unique_ptr<Object>>(makeInt(i));
    });
    m.makeFunc("*", 0, 0xffff, [](FArgs& args) {
        std::int64_t i = 1;
        double f = 1;
        bool fp = false;
        for (auto sym : args) {
            if (sym->isFloat()) {
                const double v = sym->value<double>();
                fp = true;
                i *= v;
                f *= v;
            }
            else if (sym->isInt()) {
                const std::int64_t v = sym->value<std::int64_t>();
                i *= v;
                f *= v;
            }
            else {
                throw exceptions::WrongTypeArgument(sym->toString());
            }
        }
        return fp ? static_cast<std::unique_ptr<Object>>(makeFloat(f))
            : static_cast<std::unique_ptr<Object>>(makeInt(i));
    });
    m.makeFunc("/", 1, 0xffff, [](FArgs& args) {
        std::int64_t i = 0;
        double f = 0;
        bool first = true;
        bool fp = false;
        for (auto sym : args) {
            if (sym->isFloat()) {
                const double v = sym->value<double>();
                if (v == 0) {
                    throw exceptions::ArithError("Division by zero");
                }
                fp = true;
                if (first) {
                    i = v;
                    f = v;
                }
                else {
                    i /= v;
                    f /= v;
                }
            }
            else if (sym->isInt()) {
                const std::int64_t v = sym->value<std::int64_t>();
                if (v == 0) {
                    throw exceptions::ArithError("Division by zero");
                }
                if (first) {
                    i = v;
                    f = v;
                }
                else {
                    i /= v;
                    f /= v;
                }
            }
            else {
                throw exceptions::WrongTypeArgument(sym->toString());
            }
            first = false;
        }
        return fp ? static_cast<std::unique_ptr<Object>>(makeFloat(f))
            : static_cast<std::unique_ptr<Object>>(makeInt(i));
    });

}

}
