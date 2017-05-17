#include "argParser.h"
#include <stdexcept>
#include "parseException.h"

using namespace std;
using namespace AutoArgParse;

template <>
function<string(const string&, string&)>
AutoArgParse::getArgConverter<string>() {
    return [](const string& argStr, string& dest) -> string {

        dest = argStr;
        return "";
    };
}

template <>
function<string(const string&, int&)> AutoArgParse::getArgConverter<int>() {
    return [](const string& argStr, int& dest) -> string {
        dest = 0;
        bool negated = false;
        auto stringIter = argStr.begin();
        if (*stringIter == '-') {
            negated = true;
            ++stringIter;
        }
        while (stringIter != argStr.end() && *stringIter >= '0' &&
               *stringIter <= '9') {
            dest = (dest * 10) + (*stringIter - '0');
            ++stringIter;
        }
        if (stringIter != argStr.end()) {
            return "Could not interpret \"" + argStr + "\" as an integer.";
        }
        if (negated) {
            dest = -dest;
        }
        return "";
    };
}

void AutoArgParse::ComplexFlag::parse(ArgIter& first, ArgIter& last) {
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
    if (numberParsedMandatoryFlags != numberMandatoryFlags()) {
        if (first == last) {
            throw MissingMandatoryFlagException(*this);
        } else {
            throw UnexpectedArgException(*first, *this);
        }
    }
    if (numberParsedMandatoryArgs != numberMandatoryArgs()) {
        if (first == last) {
            throw MissingMandatoryArgException(*this);
        } else {
            throw UnexpectedArgException(*first, *this);
        }
    }
    _parsed = true;
}

bool AutoArgParse::ComplexFlag::tryParseArg(ArgIter& first, ArgIter& last,
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

bool AutoArgParse::ComplexFlag::tryParseFlag(ArgIter& first, ArgIter& last,
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

typedef pair<const string, FlagPtr> FlagMapping;
void printFlag(ostream& os, const FlagMapping& flag,
               unordered_set<string>& printed, const string& prefix = " ") {
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

void AutoArgParse::ComplexFlag::invokePrintFlag(
    ostream& os, const FlagMapping& flagMapping,
    unordered_set<string>& printed) const {
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

void AutoArgParse::ComplexFlag::printUsageSummary(ostream& os) const {
    unordered_set<string> printed;
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

void AutoArgParse::Flag::printUsageHelp(ostream&, IndentedLine&) const {}

void AutoArgParse::ComplexFlag::printUsageHelp(ostream& os,
                                               IndentedLine& lineIndent) const {
    lineIndent.indentLevel++;
    lineIndent.forcePrintIndent(os);

    for (auto& argPtr : args) {
        if (argPtr->description.size() > 0) {
            os << argPtr->name;
            if (argPtr->policy == Policy::OPTIONAL) {
                os << " [optional]";
            }
            cout << ": " << argPtr->description << lineIndent;
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

void AutoArgParse::ArgParser::printSuccessfullyParsed(
    ostream& os, const char** argv, const int numberSuccessfullyParsed) const {
    for (int i = 0; i < numberSuccessfullyParsed; i++) {
        os << " " << argv[i];
    }
}

void AutoArgParse::ArgParser::validateArgs(const int argc, const char** argv,
                                           bool handleError) {
    vector<string> args(argv + 1, argv + argc);
    auto first = begin(args);
    auto last = end(args);
    try {
        parse(first, last);
        if (first != last) {
            throw UnexpectedArgException(*first, *this);
        }
        numberArgsSuccessfullyParsed = distance(first, last) + 1;
    } catch (ParseException& e) {
        numberArgsSuccessfullyParsed = distance(first, last) + 1;
        if (!handleError) {
            throw;
        }
        cerr << "Error: " << e.what() << endl;
        cerr << "Successfully parsed: ";
        printSuccessfullyParsed(cerr, argv);
        cerr << "\n\n";
        printAllUsageInfo(cerr, argv[0]);
        exit(1);
    }
}

void AutoArgParse::ArgParser::printAllUsageInfo(
    ostream& os, const string& programName) const {
    os << "Usage: " << programName;
    printUsageSummary(os);
    os << "\n\nArguments:\n";
    printUsageHelp(os);
    os << endl;
}
void AutoArgParse::throwConversionException(const string& arg,
                                            const string& additionalExpl) {
    throw ConversionException(arg, additionalExpl);
}
