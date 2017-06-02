#ifndef AUTOARGPARSE_argpAUTOARGPARSE_argparser_cpp_
#define argpAUTOARGPARSE_argparser_cpp_

#include "argParser.h"
#include <stdexcept>
#include "parseException.h"

#if AUTOARGPARSE_HEADER_ONLY
#define AUTOARGPARSE_INLINE inline
#else
#define AUTOARGPARSE_INLINE /* NO-OP */
#endif

namespace AutoArgParse {

AUTOARGPARSE_INLINE void FlagStore::parse(ArgIter& first, ArgIter& last) {
    int numberParsedMandatoryFlags = 0;
    int numberParsedMandatoryArgs = 0;
    while (first != last) {
        Policy foundPolicy;
        if (tryParseFlag(first, last, foundPolicy)) {
            if (foundPolicy == Policy::MANDATORY) {
                numberParsedMandatoryFlags++;
            }
        } else if (tryParseArg(first, last, foundPolicy)) {
            if (foundPolicy == Policy::MANDATORY) {
                numberParsedMandatoryArgs++;
            }
        } else {
            break;
        }
    }
    if (numberParsedMandatoryFlags != _numberMandatoryFlags) {
        if (first == last) {
            throw MissingMandatoryFlagException(*this);
        } else {
            throw UnexpectedArgException(*first, *this);
        }
    }
    if (numberParsedMandatoryArgs != _numberMandatoryArgs) {
        if (first == last) {
            throw MissingMandatoryArgException(*this);
        } else {
            throw UnexpectedArgException(*first, *this);
        }
    }
}

AUTOARGPARSE_INLINE bool FlagStore::tryParseArg(ArgIter& first, ArgIter& last,
                                                Policy& foundArgPolicy) {
    for (auto& argPtr : args) {
        if (argPtr->parsed()) {
            continue;
        }
        argPtr->parse(first, last);
        if (argPtr->parsed()) {
            foundArgPolicy = argPtr->policy;
            return true;
        }
    }
    return false;
}

AUTOARGPARSE_INLINE bool FlagStore::tryParseFlag(ArgIter& first, ArgIter& last,
                                                 Policy& foundFlagPolicy) {
    auto flagIter = flags.find(*first);
    if (flagIter != end(flags)) {
        if (flagIter->second->parsed()) {
            throw RepeatedFlagException(*first);
        }
        auto exclusiveIter = exclusiveFlags.find(*first);
        if (exclusiveIter != end(exclusiveFlags)) {
            if (*exclusiveIter->second) {
                throw MoreThanOneExclusiveArgException(*first, *this);
            } else {
                *exclusiveIter->second = true;
            }
        }
        ++first;
        flagIter->second->parse(first, last);
        foundFlagPolicy = flagIter->second->policy;
        return true;
    } else {
        return false;
    }
}

typedef std::pair<const std::string, FlagPtr> FlagMapping;

AUTOARGPARSE_INLINE void printFlag(std::ostream& os, const FlagMapping& flag,
                                   std::unordered_set<std::string>& printed,
                                   const std::string& prefix = " ") {
    if (!printed.count(flag.first)) {
        os << prefix;
        if (flag.second->policy == Policy::OPTIONAL) {
            os << "[";
        }
        os << flag.first;
        flag.second->printUsageSummary(os);
        if (flag.second->policy == Policy::OPTIONAL) {
            os << "]";
        }
        printed.insert(flag.first);
    }
}

AUTOARGPARSE_INLINE void FlagStore::invokePrintFlag(
    std::ostream& os, const FlagMapping& flagMapping,
    std::unordered_set<std::string>& printed) const {
    if (printed.count(flagMapping.first)) {
        return;
    }
    printFlag(os, flagMapping, printed);
    auto exclusiveFlagIter = exclusiveFlags.find(flagMapping.first);
    if (exclusiveFlagIter != end(exclusiveFlags)) {
        auto& ptr = exclusiveFlagIter->second;
        // iterating over flagInsertionOrder vector instead of simply
        // exclusiveFlags map
        // in order to print flags in the same order as they were registered.
        for (const auto& otherFlag : flagInsertionOrder) {
            auto otherExclusiveFlag = exclusiveFlags.find(otherFlag->first);
            if (otherExclusiveFlag != end(exclusiveFlags) &&
                otherExclusiveFlag->second == ptr) {
                printFlag(os, *otherFlag, printed, "|");
            }
        }
    }
    os << " ";
}

AUTOARGPARSE_INLINE void FlagStore::printUsageSummary(std::ostream& os) const {
    std::unordered_set<std::string> printed;
    for (auto& flagMappingIter : flagInsertionOrder) {
        invokePrintFlag(os, *flagMappingIter, printed);
    }
    for (auto& argPtr : args) {
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

AUTOARGPARSE_INLINE void FlagStore::printUsageHelp(
    std::ostream& os, IndentedLine& lineIndent) const {
    lineIndent.indentLevel++;
    lineIndent.forcePrintIndent(os);

    for (auto& argPtr : args) {
        if (argPtr->description.size() > 0) {
            os << argPtr->name;
            if (argPtr->policy == Policy::OPTIONAL) {
                os << " [optional]";
            }
            std::cout << ": " << argPtr->description << lineIndent;
        }
    }

    for (auto& flagMappingIter : flagInsertionOrder) {
        auto& flagMapping = *flagMappingIter;
        if (flagMapping.second->description.size() > 0) {
            os << flagMapping.first;
            flagMapping.second->printUsageSummary(os);
            os << lineIndent;
            if (flagMapping.second->policy == Policy::OPTIONAL) {
                os << "[optional] ";
            }
            os << flagMapping.second->description << lineIndent;
            flagMapping.second->printUsageHelp(os, lineIndent);
        }
    }
    lineIndent.indentLevel--;
}

AUTOARGPARSE_INLINE void ArgParser::printSuccessfullyParsed(
    std::ostream& os, const char** argv,
    const int numberSuccessfullyParsed) const {
    for (int i = 0; i < numberSuccessfullyParsed; i++) {
        os << " " << argv[i];
    }
}

AUTOARGPARSE_INLINE void ArgParser::validateArgs(const int argc,
                                                 const char** argv,
                                                 bool handleError) {
    std::vector<std::string> args(argv + 1, argv + argc);
    using std::begin;
    using std::end;
    auto first = begin(args);
    auto last = end(args);
    try {
        parse(first, last);
        if (first != last) {
            throw UnexpectedArgException(*first, this->getFlagStore());
        }
        numberArgsSuccessfullyParsed = std::distance(first, last) + 1;
    } catch (ParseException& e) {
        numberArgsSuccessfullyParsed = std::distance(first, last) + 1;
        if (!handleError) {
            throw;
        }
        std::cerr << "Error: " << e.what() << std::endl;
        std::cerr << "Successfully parsed: ";
        printSuccessfullyParsed(std::cerr, argv);
        std::cerr << "\n\n";
        printAllUsageInfo(std::cerr, argv[0]);
        exit(1);
    }
}

AUTOARGPARSE_INLINE void ArgParser::printAllUsageInfo(
    std::ostream& os, const std::string& programName) const {
    os << "Usage: " << programName;
    printUsageSummary(os);
    os << "\n\nArguments:\n";
    printUsageHelp(os);
    os << std::endl;
}

AUTOARGPARSE_INLINE void throwFailedArgConversionException(
    const std::string& name, const std::string& additionalExpl) {
    throw FailedArgConversionException(name, additionalExpl);
}

} /* namespace AutoArgParse */
#endif /* argpAUTOARGPARSE_argparser_cpp___8c26fbe7712048ea9f54d80ab26f1864 */
