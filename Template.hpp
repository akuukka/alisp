#include <type_traits>

namespace alisp {

template < template <typename...> class Template, typename T >
struct IsInstantiationOf : std::false_type {};

template < template <typename...> class Template, typename... Args >
struct IsInstantiationOf< Template, Template<Args...> > : std::true_type {};

}
