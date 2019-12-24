#ifndef AUTOARGPARSE_ARGPARSER_H_
#define AUTOARGPARSE_ARGPARSER_H_
#include "argHandlers.h"
#include "argParserBase.h"
#include "args.h"
#include "flags.h"
#include "indentedLine.h"

namespace AutoArgParse {
class ArgParser;
class PrintGroup;

class PrintGroup {
    ArgParser& argParser;
    std::string name;
    std::string description;
    std::deque<std::string> flagsToPrint;
    std::vector<size_t> argsToPrint;
    bool isDefaultGroup;

   public:
    PrintGroup(ArgParser& argParser, const std::string& name,
               const std::string& description, bool isDefaultGroup = false)
        : argParser(argParser),
          name(name),
          description(description),
          isDefaultGroup(isDefaultGroup) {}
    inline bool active() const {
        return !argsToPrint.empty() || !flagsToPrint.empty();
    }
    inline const std::string& getName() const { return name; }

    inline const std::string& getDescription() const { return description; }
    template <template <class T> class FlagType,
              typename OnParseTriggerType =
                  DoNothingTrigger>  // bool means nothing here
    typename std::enable_if<
        std::is_base_of<FlagBase, FlagType<OnParseTriggerType>>::value,
        FlagType<OnParseTriggerType>&>::type
    add(const std::string& flag, const Policy policy,
        const std::string& description,
        OnParseTriggerType&& trigger = DoNothingTrigger());

    template <typename ArgType,
              typename ConverterFunc = Converter<typename ArgType::ValueType>,
              typename ArgValueType = typename ArgType::ValueType>
    typename std::enable_if<std::is_base_of<ArgBase, ArgType>::value,
                            Arg<ArgValueType, ConverterFunc>&>::type
    add(const std::string& name, const Policy policy,
        const std::string& description,
        ConverterFunc&& convert = Converter<ArgValueType>());

    void printUsageHelp(std::ostream& os) const;
    ExclusiveFlagGroup<DoNothingTrigger>& makeExclusiveGroup(Policy);
};

class ArgParser : public ComplexFlag<DoNothingTrigger> {
    struct HelpFlagTriggeredException {};
    struct HelpFlagTrigger {
        ArgParser& argParser;
        HelpFlagTrigger(ArgParser& argParser) : argParser(argParser) {}
        void operator()(const std::string&) {
            throw HelpFlagTriggeredException();
        }
    };

    friend PrintGroup;
    int numberArgsSuccessfullyParsed = 0;
    std::vector<std::string> stringArgs;
    std::deque<PrintGroup> printGroups;
    ComplexFlag<HelpFlagTrigger>* helpFlag = NULL;
    bool firstTimePrinting = true;

   public:
    ArgParser(bool addHelpFlag = true);

    inline int getNumberArgsSuccessfullyParsed() const {
        return numberArgsSuccessfullyParsed;
    }
    void printGroupHelp(std::string groupName);
    void validateArgs(const int argc, const char** argv,
                      bool handleError = true);

    void printSuccessfullyParsed(std::ostream& os, const char** argv,
                                 int numberParsed) const;

    inline void printSuccessfullyParsed(std::ostream& os,
                                        const char** argv) const {
        printSuccessfullyParsed(os, argv, getNumberArgsSuccessfullyParsed());
    }
    void printAllUsageInfo(std::ostream& os, const std::string& programName);

    template <template <class T> class FlagType,
              typename OnParseTriggerType =
                  DoNothingTrigger>  // bool means nothing here
    typename std::enable_if<
        std::is_base_of<FlagBase, FlagType<OnParseTriggerType>>::value,
        FlagType<OnParseTriggerType>&>::type
    add(const std::string& flag, const Policy policy,
        const std::string& description,
        OnParseTriggerType&& trigger = DoNothingTrigger()) {
        return printGroups.at(0).add<FlagType>(
            flag, policy, description,
            std::forward<OnParseTriggerType>(trigger));
    }

    template <typename ArgType,
              typename ConverterFunc = Converter<typename ArgType::ValueType>,
              typename ArgValueType = typename ArgType::ValueType>
    typename std::enable_if<std::is_base_of<ArgBase, ArgType>::value,
                            Arg<ArgValueType, ConverterFunc>&>::type
    add(const std::string& name, const Policy policy,
        const std::string& description,
        ConverterFunc&& convert = Converter<ArgValueType>()) {
        return printGroups.at(0).add<ArgType>(
            name, policy, description, std::forward<ConverterFunc>(convert));
    }

    inline PrintGroup& makePrintGroup(std::string name,
                                      std::string description) {
        printGroups.emplace_back(*this, name, std::move(description));
        PrintGroup& pg = printGroups.back();
        if (helpFlag) {
            helpFlag->add<Flag>(
                name, Policy::OPTIONAL, "",
                [&pg](const std::string&) { pg.printUsageHelp(std::cout); });
        }
        return pg;
    }
};

template <template <class T> class FlagType, typename OnParseTriggerType>
typename std::enable_if<
    std::is_base_of<FlagBase, FlagType<OnParseTriggerType>>::value,
    FlagType<OnParseTriggerType>&>::type
PrintGroup::add(const std::string& flag, const Policy policy,
                const std::string& description, OnParseTriggerType&& trigger) {
    if (!isDefaultGroup) {
        flagsToPrint.emplace_back(flag);
    }
    return static_cast<ComplexFlag<DoNothingTrigger>&>(argParser).add<FlagType>(
        flag, policy, description, std::forward<OnParseTriggerType>(trigger));
}  // namespace AutoArgParse

template <typename ArgType, typename ConverterFunc, typename ArgValueType>
typename std::enable_if<std::is_base_of<ArgBase, ArgType>::value,
                        Arg<ArgValueType, ConverterFunc>&>::type
PrintGroup::add(const std::string& name, const Policy policy,
                const std::string& description, ConverterFunc&& convert) {
    if (!isDefaultGroup) {
        argsToPrint.emplace_back(argParser.getArgs().size());
    }
    return static_cast<ComplexFlag<DoNothingTrigger>&>(argParser).add<ArgType>(
        name, policy, description, std::forward<ConverterFunc>(convert));
}

ExclusiveFlagGroup<DoNothingTrigger>& PrintGroup::makeExclusiveGroup(Policy p) {
    return argParser.makeExclusiveGroup(p);
}
inline ArgParser::ArgParser(bool addHelpFlag)
    : ComplexFlag(Policy::MANDATORY, "", DoNothingTrigger()),
      printGroups({PrintGroup(*this, "default", "", true)}) {
    if (addHelpFlag) {
        helpFlag =
            &add<ComplexFlag>("--help", Policy::OPTIONAL, "Print usage help.",
                              HelpFlagTrigger(*this));
    }
}
}  // namespace AutoArgParse

#if AUTOARGPARSE_HEADER_ONLY
#include "argParser.cpp"
#endif

#endif /* AUTOARGPARSE_ARGPARSER_H_ */
