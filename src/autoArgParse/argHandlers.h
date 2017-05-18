/**This file contains types required for handling arguments.  Including parsing
 * string into types, destructor for parsed types, constraints on types, etc.*/

#ifndef AUTOARGPARSE_ARGHANDLERS_H_
#define AUTOARGPARSE_ARGHANDLERS_H_
#include <string>
namespace AutoArgParse {
struct ErrorMessage : public std::exception {
    const std::string& message;
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
struct Converter {
};

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
}

#endif /* AUTOARGPARSE_ARGHANDLERS_H_ */
