#ifndef AUTOARGPARSE_ARGPARSER_H_
#define AUTOARGPARSE_ARGPARSER_H_
#include "indentedLine.h"
#include<vector>
#include<utility>
#include<memory>
#include<string>
#include<unordered_map>
#include<vector>
#include<deque>
#include<unordered_set>
#include<iostream>
#include<stdexcept>
#include<functional>
#include<cassert>
namespace AutoArgParse {

typedef std::vector<std::string>::iterator ArgIter;

enum Policy {
    MANDATORY, OPTIONAL
};
//absolute base class for flags and args
class ParseToken {
//todo, attach actions to be executed on successful parsing of this token.
protected:
    bool _parsed;
public:
    const Policy policy;
    const std::string description;

    ParseToken(const Policy policy, const std::string& description) :
            _parsed(false), policy(policy), description(description) {
    }
    virtual ~ParseToken() = default;
    inline bool parsed() const {
        return _parsed;
    }
    inline operator bool() const {
        return parsed();
    }
};

class ComplexFlag;
class ArgBase: public ParseToken {
    friend ComplexFlag;
private:
protected:
    virtual void parse(ArgIter& first, ArgIter& last) = 0;

public:
    const std::string name;
    ArgBase(const std::string& name, const Policy policy,
            const std::string& description) :
            ParseToken(policy, description), name(name) {
    }
    virtual ~ArgBase() = default;
};

template<typename T>
std::function<std::string(const std::string&, T&)> getArgConverter() {
    assert(false);
//    static_assert(false, "Must supply a function to parse a string into the instantiated type of Arg.");
}
template<> std::function<std::string(const std::string&, std::string&)> getArgConverter<
        std::string>();

template<> std::function<std::string(const std::string&, int&)> getArgConverter<
        int>();

void throwConversionException(const std::string& arg,
        const std::string& additionalExpl);

template<typename T>
struct DefaultOnDestruct {
    inline void operator()(T&) {
        //do nothing
    }
};
template<typename T, typename onDestruct = DefaultOnDestruct<T>>
class Arg: public ArgBase {
    T parsedValue;

protected:
    virtual inline void parse(ArgIter& first, ArgIter&) {
        const std::string errorMessage = convertArgFunc(*first, parsedValue);
        this->_parsed = errorMessage.size() == 0;
        if (this->parsed()) {
            ++first;
        } else if (this->policy == Policy::MANDATORY) {
            throwConversionException(this->name, errorMessage);
        }
    }
public:
    typedef std::function<std::string(const std::string&, T&)> FunctionType;
    FunctionType convertArgFunc;

    Arg(const std::string& name, const Policy policy,
            const std::string& description, const FunctionType convertArgFunc =
                    getArgConverter<T>()) :
            ArgBase(name, policy, description), convertArgFunc(convertArgFunc) {
    }

    virtual ~Arg() {
        if (this->parsed()) {
            onDestruct()(parsedValue);
        }
    }
    T& get() {
        return parsedValue;
    }
};

class Flag: public ParseToken {
    friend ComplexFlag;

protected:
    inline virtual void parse(ArgIter&, ArgIter&) {
        //do nothing, this is a simple flag
        _parsed = true;
    }
public:
    using ParseToken::ParseToken;
    virtual void printUsageHelp(std::ostream&, IndentedLine&) const;
    inline virtual void printUsageSummary(std::ostream&) const {
    }

    inline void printUsageHelp(std::ostream& os) const {
        IndentedLine lineIndent(0);
        printUsageHelp(os, lineIndent);
    }
};

typedef std::unique_ptr<Flag> FlagPtr;
typedef std::unordered_map<std::string, FlagPtr> FlagMap;
typedef std::vector<std::unique_ptr<ArgBase>> ArgVector;
typedef std::unordered_map<std::string, std::shared_ptr<bool>> ExclusiveMap;
class ComplexFlag: public Flag {
    FlagMap flags;
    std::deque<FlagMap::iterator> flagInsertionOrder;
    ExclusiveMap exclusiveFlags;

    ArgVector args;
    int _numberMandatoryFlags;
    int _numberOptionalFlags;
    int _numberExclusiveMandatoryFlags;
    int _numberExclusiveOptionalFlags;
    int _numberMandatoryArgs;
    int _numberOptionalArgs;

    bool tryParseArg(ArgIter& first, ArgIter& last, Policy& foundArgPolicy);
    bool tryParseFlag(ArgIter& first, ArgIter& last, Policy& foundFlagPolicy);

protected:
    virtual void parse(ArgIter& first, ArgIter& last);

    void addExclusive(const std::string& flag,
            std::shared_ptr<bool>& sharedState) {
        exclusiveFlags.insert(std::make_pair(flag, sharedState));
        if (flags[flag]->policy == Policy::MANDATORY) {
            _numberExclusiveMandatoryFlags++;
        } else {
            _numberExclusiveOptionalFlags++;
        }
    }

    void invokePrintFlag(std::ostream& os,
            const std::pair<const std::string, FlagPtr>& flagMapping,
            std::unordered_set<std::string>& printed) const;
public:
    ComplexFlag(const Policy policy, const std::string& description) :
            Flag(policy, description), flags(), flagInsertionOrder(), exclusiveFlags(), args(), _numberMandatoryFlags(
                    0), _numberOptionalFlags(0), _numberExclusiveMandatoryFlags(
                    0), _numberExclusiveOptionalFlags(1), _numberMandatoryArgs(
                    0), _numberOptionalArgs(0) {
    }

    inline const ArgVector& getArgs() const {
        return args;
    }

    inline const FlagMap& getFlags() const {
        return flags;
    }

    inline const ExclusiveMap& getExclusiveFlags() const {
        return exclusiveFlags;
    }

    const auto& getFlagInsertionOrder() const {
        return flagInsertionOrder;
    }
    inline int numberMandatoryArgs() {
        return _numberMandatoryArgs;
    }

    inline int numberOptionalArgs() {
        return _numberOptionalArgs;
    }

    inline int numberMandatoryFlags() {
        return _numberMandatoryFlags - _numberExclusiveMandatoryFlags;
    }

    inline int numberOptionalFlags() {
        return _numberOptionalFlags - _numberExclusiveOptionalFlags;
    }

    template<typename FlagType, typename ... Args>
    typename std::enable_if<std::is_base_of<Flag, FlagType>::value, FlagType&>::type add(
            const std::string& flag, const Args&... args) {
        auto added = flags.insert(
                std::make_pair(flag, std::make_unique<FlagType>(args...)));
        if (added.first->second->policy == Policy::MANDATORY) {
            _numberMandatoryFlags++;
        } else {
            _numberOptionalFlags++;
        }
        flagInsertionOrder.push_back(std::move(added.first));
        //get underlying raw pointer from unique pointer, used only for casting purposes
        return *(static_cast<FlagType*>(flagInsertionOrder.back()->second.get()));
    }

    template<typename ArgType, typename ... Args>
    typename std::enable_if<std::is_base_of<ArgBase, ArgType>::value, ArgType&>::type add(
            const Args&... args) {
        this->args.emplace_back(std::make_unique<ArgType>(args...));
        if (this->args.back()->policy == Policy::MANDATORY) {
            _numberMandatoryArgs++;
        } else {
            _numberOptionalArgs++;
        }
        return *(static_cast<ArgType*>(this->args.back().get()));
    }

    template<typename ... Strings>
    std::shared_ptr<bool> makeExclusive(const Strings&... strings) {
        int previousNMandatoryExclusives = _numberExclusiveMandatoryFlags;
        int previousNOptionalExclusives = _numberExclusiveOptionalFlags;
        std::shared_ptr<bool> sharedState = std::make_shared<bool>(false);
        //code below is just to apply the same operation to every passed in arg
        int unpack[] { 0, (addExclusive(strings, sharedState), 0)... };
        static_cast<void>(unpack);
        if (_numberExclusiveMandatoryFlags != previousNMandatoryExclusives) {
            _numberExclusiveMandatoryFlags--;
        }
        if (_numberExclusiveOptionalFlags != previousNOptionalExclusives) {
            _numberExclusiveOptionalFlags--;
        }
        return sharedState;
    }

    void printUsageSummary(std::ostream& os) const;
    void printUsageHelp(std::ostream& os, IndentedLine& lineIndent) const;
    using Flag::printUsageHelp;
};

class ArgParser: public ComplexFlag {
    int numberArgsSuccessfullyParsed = 0;
public:

    ArgParser() :
            ComplexFlag(Policy::MANDATORY, ""), numberArgsSuccessfullyParsed(0) {
    }

    int getNumberArgsSuccessfullyParsed() const {
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
#endif /* AUTOARGPARSE_ARGPARSER_H_ */
