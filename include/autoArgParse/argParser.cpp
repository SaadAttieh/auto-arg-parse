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
    for (const auto& flag : flagInsertionOrder) {
        auto& flagObj = flags.at(flag);
        os << " ";

        if (flagObj->policy == Policy::OPTIONAL) {
            os << "[";
        }
        if (!flagObj->isExclusiveGroup()) {
            os << flag;
        }
        flagObj->printUsageSummary(os);
        if (flagObj->policy == Policy::OPTIONAL) {
            os << "]";
        }
    }
}

AUTOARGPARSE_INLINE void printUsageHelp(
    const std::deque<std::string>& flagInsertionOrder, const FlagMap& flags,
    std::ostream& os, IndentedLine& lineIndent) {
    for (const auto& flag : flagInsertionOrder) {
        auto& flagObj = flags.at(flag);
        if (flagObj->description.size() > 0) {
            os << lineIndent;
            os << flag;
            flagObj->printUsageSummary(os);
            os << lineIndent;
            if (flagObj->policy == Policy::OPTIONAL) {
                os << "[optional] ";
            }
            os << flagObj->description;
            flagObj->printUsageHelp(os, lineIndent);
        } else if (flagObj->isExclusiveGroup()) {
            flagObj->printUsageHelp(os, lineIndent);
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
    AutoArgParse::printUsageHelp(flagInsertionOrder, flags, os, lineIndent);
    lineIndent.indentLevel--;
}

AUTOARGPARSE_INLINE void PrintGroup::printUsageHelp(std::ostream& os) const {
    IndentedLine lineIndent(0);
    if (isDefaultGroup) {
        argParser.printUsageHelp(os, lineIndent);
        return;
    }
    for (size_t index : argsToPrint) {
        auto& argPtr = argParser.getArgs()[index];
        if (argPtr->description.size() > 0) {
            os << lineIndent;
            os << argPtr->name;
            if (argPtr->policy == Policy::OPTIONAL) {
                os << " [optional]";
            }
            std::cout << ": " << argPtr->description;
        }
    }
    AutoArgParse::printUsageHelp(flagsToPrint, argParser.store.flags, os,
                                 lineIndent);
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
    if (helpFlag) {
        // help flag would have been the first thing added, move it to the end
        store.rotateLeft();
    }
    auto first = begin(stringArgs);
    auto last = end(stringArgs);
    try {
        parse(first, last);
        if (first != last) {
            throw UnexpectedArgException(*first, this->getFlagStore());
        }
        numberArgsSuccessfullyParsed =
            std::distance(begin(stringArgs), first) + 1;
    } catch (ParseException& e) {
        numberArgsSuccessfullyParsed =
            std::distance(begin(stringArgs), first) + 1;
        if (!handleError) {
            throw;
        }
        std::cerr << "Error: " << e.what() << std::endl;
        std::cerr << "Successfully parsed: ";
        printSuccessfullyParsed(std::cerr, argv);
        std::cerr << "\n\n";
        printAllUsageInfo(std::cerr, argv[0]);
        exit(1);
    } catch (HelpFlagTriggeredException& e) {
        if (first[-1] == std::string("--help")) {
            printAllUsageInfo(std::cout, argv[0]);
        }
    }
}

AUTOARGPARSE_INLINE void ArgParser::printAllUsageInfo(
    std::ostream& os, const std::string& programName) const {
    os << "Usage: " << programName;
    printUsageSummary(os);
    os << "\n\nArguments:\n";
    if (printGroups.size() == 1) {
        printGroups.at(0).printUsageHelp(os);
        return;
    }
    os << "The options are divided into the following groups.  Use --help "
          "group_name to detail the options under a particular group:\n";
    bool first = true;
    for (auto& pg : printGroups) {
        if (first) {
            first = false;
            continue;
        }
        os << pg.getName() << "  -- " << pg.getDescription() << std::endl;
    }
}

AUTOARGPARSE_INLINE void throwFailedArgConversionException(
    const std::string& name, const std::string& additionalExpl) {
    throw FailedArgConversionException(name, additionalExpl);
}

AUTOARGPARSE_INLINE void throwMoreThanOneExclusiveArgException(
    const std::string& conflictingFlag1, const std::string& conflictingFlag2,
    const std::deque<std::string>& exclusiveFlags) {
    throw MoreThanOneExclusiveArgException(conflictingFlag1, conflictingFlag2,
                                           exclusiveFlags);
}

AUTOARGPARSE_INLINE void FlagStore::rotateLeft() {
    if (flagInsertionOrder.empty()) {
        return;
    }
    auto flag = std::move(flagInsertionOrder.front());
    flagInsertionOrder.pop_front();
    flagInsertionOrder.emplace_back(std::move(flag));
}
} /* namespace AutoArgParse */
#endif /* AUTOARGPARSE_ARGPARSER_CPP_ */
