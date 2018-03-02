/**This file contains types required for handling arguments.  Including parsing
 strings into types and constraints on types.*/

#ifndef AUTOARGPARSE_ARGHANDLERS_H_
#define AUTOARGPARSE_ARGHANDLERS_H_
#include <string>
#include <tuple>
#include <utility>
namespace AutoArgParse {
struct ErrorMessage : public std::exception {
    const std::string message;
    ErrorMessage(const std::string& message) : message(message) {}
};

/** a fake converter object, should never be called, used only to assist with
meta programming. */ template <typename T>
struct FakeDoNothingConverter {
    template <typename... Args>
    T operator()(const Args&...) const {
        abort();
    }
};

/**
 * Default object for converting strings to types.  Cannot be instantiated, must
be specialised.
*/
template <typename T>
struct Converter {};

template <>
struct Converter<std::string> {
    inline std::string operator()(const std::string& stringArgToParse) const {
        return stringArgToParse;
    }
};

template <>
struct Converter<int> {
    inline int operator()(const std::string& stringArgToParse) const {
        int parsedInt = 0;
        bool negated = false;
        auto stringIter = stringArgToParse.begin();
        if (*stringIter == '-') {
            negated = true;
            ++stringIter;
        }
        while (stringIter != stringArgToParse.end() && *stringIter >= '0' &&
               *stringIter <= '9') {
            parsedInt = (parsedInt * 10) + (*stringIter - '0');
            ++stringIter;
        }
        if (stringIter != stringArgToParse.end()) {
            throw AutoArgParse::ErrorMessage("Could not interpret \"" +
                                             stringArgToParse +
                                             "\" as an integer.");
        }
        if (negated) {
            parsedInt = -parsedInt;
        }
        return parsedInt;
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
