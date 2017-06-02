#ifndef AUTOARGPARSE_PARSEEXCEPTION_H_
#define AUTOARGPARSE_PARSEEXCEPTION_H_
#include <cassert>
#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include "argParser.h"
namespace AutoArgParse {
template <typename FlagContainer>
void printUnParsed(std::ostringstream& os, const FlagContainer& flags) {
    bool first = true;
    for (auto& flag : flags) {
        if (flag->second->isExclusiveGroup()) {
            printUnParsed(os, flag->second->getFlags());
            continue;
        }
        if (!flag->second->parsed() && flag->second->available()) {
            if (first) {
                os << " ";
                first = false;
            } else {
                os << ", ";
            }
            if (flag->second->policy == Policy::OPTIONAL) {
                os << "[";
            }
            os << flag->first;
            if (flag->second->policy == Policy::OPTIONAL) {
                os << "]";
            }
        }
    }
}

void printUnParsed(std::ostringstream& os, const ArgVector& args) {
    for (auto& argPtr : args) {
        if (!argPtr->parsed()) {
            os << " ";
            if (argPtr->policy == Policy::OPTIONAL) {
                os << "[";
            }
            os << argPtr->name;
            if (argPtr->policy == Policy::OPTIONAL) {
                os << "]";
            }
        }
    }
}

enum ParseFailureReason {
    MISSING_MANDATORY_FLAG,
    REPEATED_FLAG,
    MISSING_MANDATORY_ARG,
    UNEXPECTED_ARG,
    MORE_THAN_ONE_EXCLUSIVE_ARG,
    FAILED_ARG_CONVERSION
};

class ParseException : public std::exception {
   public:
    const ParseFailureReason failureReason;
    const std::string errorMessage;

    ParseException(ParseFailureReason failureReason, std::string errorMessage)
        : failureReason(failureReason), errorMessage(errorMessage) {}

    virtual ~ParseException() = default;

    virtual const char* what() const noexcept { return errorMessage.c_str(); }
};

class MissingMandatoryArgException : public ParseException {
   public:
    const FlagStore& flagStore;

    MissingMandatoryArgException(const FlagStore& flagStore)
        : ParseException(MISSING_MANDATORY_ARG, makeErrorMessage(flagStore)),
          flagStore(flagStore) {}
    static std::string makeErrorMessage(const FlagStore& flagStore) {
        std::ostringstream os;
        os << "Missing mandatory argument(s).  Valid option(s) are: ";
        printUnParsed(os, flagStore.args);
        return os.str();
    }
};

class MissingMandatoryFlagException : public ParseException {
   public:
    const FlagStore& flagStore;

    MissingMandatoryFlagException(const FlagStore& flagStore)
        : ParseException(MISSING_MANDATORY_FLAG, makeErrorMessage(flagStore)),
          flagStore(flagStore) {}
    static std::string makeErrorMessage(const FlagStore& flagStore) {
        std::ostringstream os;
        os << "Missing mandatory argument(s). valid option(s) are: ";
        printUnParsed(os, flagStore.flagInsertionOrder);
        return os.str();
    }
};

class RepeatedFlagException : public ParseException {
   public:
    const std::string repeatedFlag;
    RepeatedFlagException(std::string repeatedFlag)
        : ParseException(REPEATED_FLAG, makeErrorMessage(repeatedFlag)),
          repeatedFlag(std::move(repeatedFlag)) {}
    static std::string makeErrorMessage(const std::string& flag) {
        return "Repeated flag: " + flag;
    }
};

class UnexpectedArgException : public ParseException {
   public:
    const std::string unexpectedArg;
    const FlagStore& flagStore;

    UnexpectedArgException(std::string unexpectedArg,
                           const FlagStore& flagStore)
        : ParseException(UNEXPECTED_ARG,
                         makeErrorMessage(unexpectedArg, flagStore)),
          unexpectedArg(std::move(unexpectedArg)),
          flagStore(flagStore) {}
    static std::string makeErrorMessage(const std::string& unexpectedArg,
                                        const FlagStore& flagStore) {
        std::ostringstream os;
        os << "Unexpected argument: " << unexpectedArg << std::endl;
        os << "Valid option(s): ";
        printUnParsed(os, flagStore.flagInsertionOrder);
        printUnParsed(os, flagStore.args);
        return os.str();
    }
};

class MoreThanOneExclusiveArgException : public ParseException {
   public:
    const std::string conflictingFlag1;
    const std::string conflictingFlag2;
    const std::vector<FlagMap::iterator>& exclusiveFlags;

    MoreThanOneExclusiveArgException(
        const std::string& conflictingFlag1,
        const std::string& conflictingFlag2,
        const std::vector<FlagMap::iterator>& exclusiveFlags)
        : ParseException(MORE_THAN_ONE_EXCLUSIVE_ARG,
                         makeErrorMessage(conflictingFlag1, conflictingFlag2,
                                          exclusiveFlags)),
          conflictingFlag1(conflictingFlag1),
          conflictingFlag2(conflictingFlag2),
          exclusiveFlags(exclusiveFlags) {}
    static std::string makeErrorMessage(
        const std::string& conflictingFlag1,
        const std::string& conflictingFlag2,
        const std::vector<FlagMap::iterator>& exclusiveFlags) {
        std::ostringstream os;
        os << "Cannot use " << conflictingFlag1 << " in conjunction with "
           << conflictingFlag2 << ".\nThe following flags are exclusive:\n";
        bool first = true;
        for (const auto& flagIter : exclusiveFlags) {
            if (first) {
                first = false;
            } else {
                os << "|";
            }
            os << flagIter->first;
        }
        os << "\n";
        return os.str();
    }
};

class FailedArgConversionException : public ParseException {
   public:
    const std::string argName;
    const std::string additionalExpl;
    FailedArgConversionException(const std::string& argName,
                                 const std::string& additionalExpl)
        : ParseException(FAILED_ARG_CONVERSION,
                         makeErrorMessage(argName, additionalExpl)),
          argName(argName),
          additionalExpl(additionalExpl) {}
    static std::string makeErrorMessage(const std::string& argName,
                                        const std::string& additionalExpl) {
        return "Could not parse argument: " + argName + "\n" + additionalExpl +
               "\n";
    }
};
}
#endif /* AUTOARGPARSE_PARSEEXCEPTION_H_ */
