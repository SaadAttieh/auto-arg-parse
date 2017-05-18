/**This file contains types required for handling arguments.  Including parsing
 * string into types, destructor for parsed types, constraints on types, etc.*/

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
    void operator()(const T&... args) const;
};

/**
 *Empty object used just to cause the type to appear after a static assert.
 */
template <typename T>
struct Reveal {};

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
template <typename T>
struct Constraint {
    inline void operator()(const T&) const {}
};
/**
 * Class for chaining multiple constraints together, allowing multiple
 * constraints to be attached to one arg.
 */
template <typename... Constraints>
class ConstraintChain {
    const std::tuple<Constraints...> constraints;
    ConstraintChain(const Constraints&... constraintsIn)
        : constraints(std::tie(constraintsIn...)) {}
    template <std::size_t I = 0, typename T>
    inline typename std::enable_if<I == sizeof...(Constraints), void>::type
    operator()(const T&) {}

    template <std::size_t I = 0, typename T>
        inline typename std::enable_if <
        I<sizeof...(Constraints), void>::type operator()(const T& parsedValue) {
        std::get<I>(constraints)(parsedValue);
        operator()(I + 1, parsedValue);
    }
};

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
    void operator()(int parsedValue) const {
        int testMin = (minInclusive) ? min : min + 1;
        int testMax = (maxInclusive) ? max : max - 1;
        if (parsedValue < testMin || parsedValue > testMax) {
            throw ErrorMessage("Expected value to be between " + std::to_string(min) +
                    ((minInclusive) ? "(inclusive)" : "(exclusive") + " and " +
                    std::to_string(max) +
                    ((maxInclusive) ? "(inclusive)" : "(exclusive)") + ".");
        }
    }
};
}

#endif /* AUTOARGPARSE_ARGHANDLERS_H_ */
