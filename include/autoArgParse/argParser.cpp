#ifndef AUTOARGPARSE_ARGPARSER_CPP_
#define AUTOARGPARSE_ARGPARSER_CPP_

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
        ++first;
        flagIter->second->parse(first, last);
        foundFlagPolicy = flagIter->second->policy;
        return true;
    } else {
        return false;
    }
}

typedef std::pair<const std::string, FlagPtr> FlagMapping;

AUTOARGPARSE_INLINE void FlagStore::printUsageSummary(std::ostream& os) const {
    for (const auto& flagMappingIter : flagInsertionOrder) {
        os << " ";
        if (flagMappingIter->second->policy == Policy::OPTIONAL) {
            os << "[";
        }
        if (!flagMappingIter->second->isExclusiveGroup()) {
            os << flagMappingIter->first;
        }
        flagMappingIter->second->printUsageSummary(os);
        if (flagMappingIter->second->policy == Policy::OPTIONAL) {
            os << "]";
        }
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

AUTOARGPARSE_INLINE void printUsageHelp(
    const std::vector<FlagMap::iterator>& flagInsertionOrder, std::ostream& os,
    IndentedLine& lineIndent) {
    for (const auto& flagMappingIter : flagInsertionOrder) {
        auto& flagMapping = *flagMappingIter;
        if (flagMapping.second->description.size() > 0) {
            os << lineIndent;
            os << flagMapping.first;
            flagMapping.second->printUsageSummary(os);
            os << lineIndent;
            if (flagMapping.second->policy == Policy::OPTIONAL) {
                os << "[optional] ";
            }
            os << flagMapping.second->description;
            flagMapping.second->printUsageHelp(os, lineIndent);
        } else if (flagMapping.second->isExclusiveGroup()) {
            flagMapping.second->printUsageHelp(os, lineIndent);
        }
    }
}

AUTOARGPARSE_INLINE void FlagStore::printUsageHelp(
    std::ostream& os, IndentedLine& lineIndent) const {
    lineIndent.indentLevel++;
    for (auto& argPtr : args) {
        if (argPtr->description.size() > 0) {
            os << lineIndent;
            os << argPtr->name;
            if (argPtr->policy == Policy::OPTIONAL) {
                os << " [optional]";
            }
            std::cout << ": " << argPtr->description;
        }
    }
    AutoArgParse::printUsageHelp(flagInsertionOrder, os, lineIndent);
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
    stringArgs.assign(argv + 1, argv + argc);
    using std::begin;
    using std::end;
    auto first = begin(stringArgs);
    auto last = end(stringArgs);
    try {
        parse(first, last);
        if (first != last) {
            throw UnexpectedArgException(*first, this->getFlagStore());
        }
        numberArgsSuccessfullyParsed = std::distance(begin(stringArgs), first) + 1;
    } catch (ParseException& e) {
        numberArgsSuccessfullyParsed = std::distance(begin(stringArgs), first) + 1;
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

AUTOARGPARSE_INLINE void throwMoreThanOneExclusiveArgException(
    const std::string& conflictingFlag1, const std::string& conflictingFlag2,
    const std::vector<FlagMap::iterator>& exclusiveFlags) {
    throw MoreThanOneExclusiveArgException(conflictingFlag1, conflictingFlag2,
                                           exclusiveFlags);
    ;
}

} /* namespace AutoArgParse */
#endif /* AUTOARGPARSE_ARGPARSER_CPP_ */
