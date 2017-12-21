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

/** helper functions for ConverterChain class below */

template <
    std::size_t I = 0, typename T, typename... ConverterChain,
    typename std::enable_if<I == sizeof...(ConverterChain), int>::type = 0>
inline auto runChain(T&& converted, std::tuple<ConverterChain...>&)
    -> decltype(std::forward<T>(converted)) {
    return std::forward<T>(converted);
}

template <std::size_t I = 0, typename T, typename... ConverterChain,
          typename std::enable_if<I<sizeof...(ConverterChain), int>::type =
                                      0> inline auto
              runChain(T&& parsingValue,
                       std::tuple<ConverterChain...>& converterChain)
                  ->decltype(runChain<I + 1>(std::get<I>(converterChain)(
                                                 std::forward<T>(parsingValue)),
                                             converterChain)) {
    return runChain<I + 1>(
        std::get<I>(converterChain)(std::forward<T>(parsingValue)),
        converterChain);
}

/**
 * Class for chaining multiple converterstogether, allowing multiple
 * converters or constraints to be attached to one arg.
 */
template <typename... ConverterChain>
class Chain {
    std::tuple<ConverterChain...> converterChain;

   public:
    Chain(ConverterChain&&... converterChainIn)
        : converterChain(std::forward<ConverterChain>(converterChainIn)...) {}

    template <typename T>
    inline auto operator()(T&& parsingValue) -> decltype(
        runChain<0>(std::forward<T>(parsingValue),
                    std::declval<std::tuple<ConverterChain...>&>())) {
        return runChain<0>(std::forward<T>(parsingValue), converterChain);
    }
};

template <typename... Converters>
Chain<Converters...> chain(Converters&&... converters) {
    return Chain<Converters...>(std::forward<Converters>(converters)...);
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
