/**This file contains types required for handling arguments.  Including parsing
 strings into types and constraints on types.*/

#ifndef AUTOARGPARSE_ARGHANDLERS_H_
#define AUTOARGPARSE_ARGHANDLERS_H_
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
namespace AutoArgParse {
struct ErrorMessage : public std::exception {
    const std::string message;
    ErrorMessage(const std::string& message) : message(message) {}
};

/** a fake converter object, should never be called, used only to assist with
meta programming. */
template <typename T>
struct FakeDoNothingConverter {
    template <typename... Args>
    T operator()(const Args&...) const {
        abort();
    }
};

/**
 * Default object for converting strings to types.  Defaults to using
 * `istringstream` and the `>>` operator. can be specialised to use custom
 * parser.
 */
template <typename T>
struct Converter {
    inline T operator()(const std::string& stringArgToParse) const {
        T value;
        std::istringstream is(stringArgToParse);
        bool success(is >> value);
        if (!success) {
            throw ErrorMessage(makeErrorMessage());
        }
        return value;
    }

    // allow custom error messages for integral, float and unsigned types
    static inline std::string makeErrorMessage() {
        std::ostringstream os;
        if (std::is_integral<T>::value) {
            os << "Could not interpret argument as integer";
        } else if (std::is_floating_point<T>::value) {
            os << "Could not interpret argument as number";
        } else {
            os << "Could not parse argument";
        }
        if (std::is_unsigned<T>::value) {
            os << " greater or equal to 0.";
        } else {
            os << ".";
        }
        return os.str();
    }
};

template <>
struct Converter<std::string> {
    inline std::string operator()(const std::string& stringArgToParse) const {
        return stringArgToParse;
    }
};

/*helper classes for the chain function, a function that composes multiple
 * functions together*/
namespace detail {

/** helper functions for ConverterChain class below */
template <typename Func1, typename Func2>
class Composed {
    Func1 func1;
    Func2 func2;

   public:
    Composed(Func1&& func1, Func2&& func2)
        : func1(std::forward<Func1>(func1)),
          func2(std::forward<Func2>(func2)) {}
    template <typename T>
    inline auto operator()(T&& arg)
        -> decltype(func2(func1(std::forward<T>(arg)))) {
        return func2(func1(std::forward<T>(arg)));
    }
};
template <typename Func1, typename Func2>
inline Composed<Func1, Func2> composed(Func1&& func1, Func2&& func2) {
    return Composed<Func1, Func2>(std::forward<Func1>(func1),
                                  std::forward<Func2>(func2));
}

template <typename... Funcs>
struct ChainType;

template <typename Func1, typename Func2>
struct ChainType<Func1, Func2> {
    typedef Composed<Func1, Func2> type;
};

template <typename Func1, typename Func2, typename... Funcs>
struct ChainType<Func1, Func2, Funcs...> {
    typedef Composed<Func1, typename ChainType<Func2, Funcs...>::type> type;
};
}  // namespace detail

template <typename Func>
inline auto chain(Func&& func) -> decltype(std::forward<Func>(func)) {
    return std::forward<Func>(func);
}

template <typename Func, typename... Funcs>
inline typename detail::ChainType<Func, Funcs...>::type chain(
    Func&& func, Funcs&&... funcs) {
    return detail::composed(std::forward<Func>(func),
                            chain(std::forward<Funcs>(funcs)...));
}

/**
 * Integer range constraint
 */
class IntRange {
   public:
    const int min, max;
    const bool minInclusive, maxInclusive;
    IntRange(int min, int max, bool minInclusive, bool maxInclusive)
        : min(min),
          max(max),
          minInclusive(minInclusive),
          maxInclusive(maxInclusive) {}
    int operator()(int parsedValue) const {
        int testMin = (minInclusive) ? min : min + 1;
        int testMax = (maxInclusive) ? max : max - 1;
        if (parsedValue < testMin || parsedValue > testMax) {
            throw ErrorMessage(
                "Expected value to be between " + std::to_string(min) +
                ((minInclusive) ? "(inclusive)" : "(exclusive") + " and " +
                std::to_string(max) +
                ((maxInclusive) ? "(inclusive)" : "(exclusive)") + ".");
        }
        return parsedValue;
    }
};
}  // namespace AutoArgParse

#endif /* AUTOARGPARSE_ARGHANDLERS_H_ */
