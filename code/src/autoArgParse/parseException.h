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
void printUnParsed(std::ostringstream& os, const FlagContainer& flags,
                   const ExclusiveMap& exclusiveFlags) {
    bool first = true;
    for (auto& flag : flags) {
        if (!flag->second->parsed() &&
            !(exclusiveFlags.count(flag->first) &&
              *exclusiveFlags.find(flag->first)->second)) {
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
    CONVERSION_ERROR
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
    const ComplexFlag& complexFlag;

    MissingMandatoryArgException(const ComplexFlag& complexFlag)
        : ParseException(MISSING_MANDATORY_ARG, makeErrorMessage(complexFlag)),
          complexFlag(complexFlag) {}
    static std::string makeErrorMessage(const ComplexFlag& complexFlag) {
        std::ostringstream os;
        os << "Missing mandatory argument(s).  Valid option(s) are: ";
        printUnParsed(os, complexFlag.getArgs());
        return os.str();
    }
};

class MissingMandatoryFlagException : public ParseException {
   public:
    const ComplexFlag& complexFlag;

    MissingMandatoryFlagException(const ComplexFlag& complexFlag)
        : ParseException(MISSING_MANDATORY_FLAG, makeErrorMessage(complexFlag)),
          complexFlag(complexFlag) {}
    static std::string makeErrorMessage(const ComplexFlag& complexFlag) {
        std::ostringstream os;
        os << "Missing mandatory argument(s). valid option(s) are: ";
        printUnParsed(os, complexFlag.getFlagInsertionOrder(),
                      complexFlag.getExclusiveFlags());
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
    const ComplexFlag& complexFlag;

    UnexpectedArgException(std::string unexpectedArg,
                           const ComplexFlag& complexFlag)
        : ParseException(UNEXPECTED_ARG,
                         makeErrorMessage(unexpectedArg, complexFlag)),
          unexpectedArg(std::move(unexpectedArg)),
          complexFlag(complexFlag) {}
    static std::string makeErrorMessage(const std::string& unexpectedArg,
                                        const ComplexFlag& complexFlag) {
        std::ostringstream os;
        os << "Unexpected argument: " << unexpectedArg << std::endl;
        os << "Valid option(s): ";
        printUnParsed(os, complexFlag.getFlagInsertionOrder(),
                      complexFlag.getExclusiveFlags());
        printUnParsed(os, complexFlag.getArgs());
        return os.str();
    }
};

class MoreThanOneExclusiveArgException : public ParseException {
   public:
    const std::string exclusiveArg;
    const ComplexFlag& complexFlag;

    MoreThanOneExclusiveArgException(std::string exclusiveArg,
                                     const ComplexFlag& complexFlag)
        : ParseException(MORE_THAN_ONE_EXCLUSIVE_ARG,
                         makeErrorMessage(exclusiveArg, complexFlag)),
          exclusiveArg(exclusiveArg),
          complexFlag(complexFlag) {}
    static std::string makeErrorMessage(const std::string& exclusiveArg,
                                        const ComplexFlag& complexFlag) {
        std::ostringstream os;
        os << "The following arguments are exclusive and may not be used in "
              "conjunction: ";
        auto mappingIter = complexFlag.getExclusiveFlags().find(exclusiveArg);
        assert(mappingIter != std::end(complexFlag.getExclusiveFlags()));
        auto& ptr = mappingIter->second;
        bool first = true;
        for (auto& mapping : complexFlag.getExclusiveFlags()) {
            if (mapping.second == ptr) {
                if (first) {
                    first = false;
                } else {
                    os << "|";
                }
                os << mapping.first;
            }
        }
        return os.str();
    }
};

class ConversionException : public ParseException {
   public:
    const std::string argName;
    const std::string additionalExpl;
    ConversionException(const std::string& argName,
                        const std::string& additionalExpl)
        : ParseException(CONVERSION_ERROR,
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
