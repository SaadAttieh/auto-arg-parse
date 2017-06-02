/**This file contains types required for handling arguments.  Including parsing
 strings into types and constraints on types.*/

#ifndef AUTOARGPARSE_ARGHANDLERS_H_
#define AUTOARGPARSE_ARGHANDLERS_H_
#include <string>
namespace AutoArgParse {
struct ErrorMessage : public std::exception {
    const std::string message;
    ErrorMessage(const std::string& message) : message(message) {}
};
/**
 *abstract empty object that supports any function interface, used only as
 *default values to
 *templates
 */
struct DefaultDoNothingHandler {
    template <typename... T>
    inline void operator()(const T&... args) const {}
};

/**
 * Default object for converting strings to types.  Cannot be instantiated, must
be specialised.
*/
template <typename T>
struct Converter {};

template <>
struct Converter<std::string> {
    inline void operator()(const std::string& stringArgToParse,
                           std::string& parsedString) const {
        parsedString = stringArgToParse;
    }
};

template <>
struct Converter<int> {
    inline void operator()(const std::string& stringArgToParse,
                           int& parsedInt) const {
        parsedInt = 0;
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
    }
};

/**
 * Default constraint on arguments, i.e. no constraint.
 */

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

    template <std::size_t I = 0, typename T>
    inline typename std::enable_if<I == sizeof...(ConverterChain), void>::type
    operator()(const std::string&, T&) {}

    template <std::size_t I = 0, typename T>
        inline typename std::enable_if <
        I<sizeof...(ConverterChain), void>::type operator()(
            const std::string& stringToParse, T& parsedValue) {
        std::get<I>(converterChain)(stringToParse, parsedValue);
        operator()<I + 1>(stringToParse, parsedValue);
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
    void operator()(const std::string&, int parsedValue) const {
        int testMin = (minInclusive) ? min : min + 1;
        int testMax = (maxInclusive) ? max : max - 1;
        if (parsedValue < testMin || parsedValue > testMax) {
            throw ErrorMessage(
                "Expected value to be between " + std::to_string(min) +
                ((minInclusive) ? "(inclusive)" : "(exclusive") + " and " +
                std::to_string(max) +
                ((maxInclusive) ? "(inclusive)" : "(exclusive)") + ".");
        }
    }
};
}

#endif /* AUTOARGPARSE_ARGHANDLERS_H_ */
