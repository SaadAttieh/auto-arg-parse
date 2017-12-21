#ifndef AUTOARGPARSE_ARGPARSER_H_
#define AUTOARGPARSE_ARGPARSER_H_
#include "argHandlers.h"
#include "argParserBase.h"

#include "args.h"
#include "flags.h"
#include "indentedLine.h"

namespace AutoArgParse {

class ArgParser : public ComplexFlag<DoNothingTrigger> {
    int numberArgsSuccessfullyParsed = 0;
    std::vector<std::string> stringArgs;

   public:
    ArgParser() : ComplexFlag(Policy::MANDATORY, "", DoNothingTrigger()) {}

    inline int getNumberArgsSuccessfullyParsed() const {
        return numberArgsSuccessfullyParsed;
    }
    void validateArgs(const int argc, const char** argv,
                      bool handleError = true);

    void printSuccessfullyParsed(std::ostream& os, const char** argv,
                                 int numberParsed) const;

    inline void printSuccessfullyParsed(std::ostream& os,
                                        const char** argv) const {
        printSuccessfullyParsed(os, argv, getNumberArgsSuccessfullyParsed());
    }
    void printAllUsageInfo(std::ostream& os,
                           const std::string& programName) const;
};
}

#if AUTOARGPARSE_HEADER_ONLY
#include "argParser.cpp"
#endif

#endif /* AUTOARGPARSE_ARGPARSER_H_ */
