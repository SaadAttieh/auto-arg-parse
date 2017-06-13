
#ifndef AUTOARGPARSE_FLAGS_H_
#define AUTOARGPARSE_FLAGS_H_
#include <deque>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "argHandlers.h"
#include "argParserBase.h"

namespace AutoArgParse {
// some constants
static const char* alphanumericSymbols =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890";
static const size_t alphanumericSymbolsLength = strlen(alphanumericSymbols);
static const int RANDOM_STRING_LENGTH = 10;
inline std::string generateRandomString(size_t length) {
    auto generator = [&]() {
        return alphanumericSymbols[rand() % alphanumericSymbolsLength];
    };
    std::string result;
    result.reserve(length);
    generate_n(back_inserter(result), length, generator);
    return result;
}
static const std::string RANDOM_STRING =
    generateRandomString(RANDOM_STRING_LENGTH);

/**
 * Represents a simple flag, does not have any nested flags or arguments.
 */
template <typename OnParseTrigger>
class Flag : public FlagBase {
    OnParseTrigger parsedTrigger;

   protected:
    inline void triggerParseSuccess(const std::string& flag) {
        parsedTrigger(flag);
    }
    inline virtual void parse(ArgIter& first, ArgIter&) {
        _parsed = true;
        triggerParseSuccess(first[-1]);
        // we know that first always points to the element just after the most
        // recently parsed flag
    }

   public:
    Flag(const Policy policy, const std::string& description,
         OnParseTrigger&& trigger)
        : FlagBase(policy, description),
          parsedTrigger(std::forward<OnParseTrigger>(trigger)) {}
    virtual ~Flag() = default;
};

void printUsageHelp(const std::vector<FlagMap::iterator>& flagInsertionOrder,
                    std::ostream& os, IndentedLine& lineIndent);

/**
 * A non templated object that can hold most of the data belonging to templated
 * class ComplexFlag and implement some of its functions.  The purpose of this
 * class is to allow non templated functions to be defined in the CPP file
 * rather than having to inline the code in this file.
 */
class FlagStore {
   public:
    FlagMap flags;
    std::vector<FlagMap::iterator> flagInsertionOrder;
    ArgVector args;
    int _numberMandatoryFlags = 0;
    int _numberOptionalFlags = 0;
    int _numberMandatoryArgs = 0;
    int _numberOptionalArgs = 0;

    bool tryParseArg(ArgIter& first, ArgIter& last, Policy& foundArgPolicy);
    bool tryParseFlag(ArgIter& first, ArgIter& last, Policy& foundFlagPolicy);

    void parse(ArgIter& first, ArgIter& last);
    void printUsageSummary(std::ostream& os) const;
    virtual void printUsageHelp(std::ostream& os,
                                IndentedLine& lineIndent) const;
};

void throwMoreThanOneExclusiveArgException(
    const std::string& conflictingFlag1, const std::string& conflictingFlag2,
    const std::vector<FlagMap::iterator>& exclusiveFlags);

template <typename T>
class ExclusiveFlagGroup;

template <typename OnParseTrigger>
class ComplexFlag : public Flag<OnParseTrigger> {
    friend class ExclusiveFlagGroup<OnParseTrigger>;

    FlagStore store;

   protected:
    virtual void parse(ArgIter& first, ArgIter& last) {
        int distance = (int)std::distance(first, last);
        store.parse(first, last);
        this->_parsed = true;
        this->triggerParseSuccess(last[(0 - distance) - 1]);
    }

    inline const FlagStore& getFlagStore() { return store; }

   public:
    using Flag<OnParseTrigger>::Flag;

    inline const ArgVector& getArgs() const { return store.args; }

    inline const FlagMap& getFlags() const { return store.flags; }

    const std::deque<FlagMap::iterator>& getFlagInsertionOrder() const {
        return store.flagInsertionOrder;
    }

    inline int numberMandatoryArgs() { return store._numberMandatoryArgs; }

    inline int numberOptionalArgs() { return store._numberOptionalArgs; }

    inline int numberMandatoryFlags() { return store._numberMandatoryFlags; }

    inline int numberOptionalFlags() { return store._numberOptionalFlags; }

    ExclusiveFlagGroup<OnParseTrigger>& makeExclusiveGroup(Policy);
    template <template <class T> class FlagType,
              typename OnParseTriggerType = DefaultDoNothingHandler>
    typename std::enable_if<
        std::is_base_of<FlagBase, FlagType<OnParseTriggerType>>::value,
        FlagType<OnParseTriggerType>&>::type
    add(const std::string& flag, const Policy policy,
        const std::string& description,
        OnParseTriggerType&& trigger = DefaultDoNothingHandler()) {
        auto added = store.flags.insert(std::make_pair(
            flag, std::unique_ptr<FlagBase>(new FlagType<OnParseTriggerType>(
                      policy, description,
                      std::forward<OnParseTriggerType>(trigger)))));
        if (added.first->second->policy == Policy::MANDATORY) {
            ++store._numberMandatoryFlags;
        } else {
            ++store._numberOptionalFlags;
        }
        store.flagInsertionOrder.push_back(std::move(added.first));
        // get underlying raw pointer from unique pointer, used only for casting
        // purposes
        return *(static_cast<FlagType<OnParseTriggerType>*>(
            store.flagInsertionOrder.back()->second.get()));
    }

    template <typename ArgType,
              typename ConverterFunc = Converter<typename ArgType::ValueType>,
              typename ArgValueType = typename ArgType::ValueType>
    typename std::enable_if<std::is_base_of<ArgBase, ArgType>::value,
                            Arg<ArgValueType, ConverterFunc>&>::type
    add(const std::string& name, const Policy policy,
        const std::string& description,
        ConverterFunc&& convert = Converter<ArgValueType>()) {
        store.args.emplace_back(
            std::unique_ptr<Arg<ArgValueType, ConverterFunc>>(
                new Arg<ArgValueType, ConverterFunc>(
                    name, policy, description,
                    std::forward<ConverterFunc>(convert))));
        if (store.args.back()->policy == Policy::MANDATORY) {
            ++store._numberMandatoryArgs;
        } else {
            ++store._numberOptionalArgs;
        }
        return *(static_cast<Arg<ArgValueType, ConverterFunc>*>(
            store.args.back().get()));
    }

    inline void printUsageSummary(std::ostream& os) const {
        store.printUsageSummary(os);
    }
    inline void printUsageHelp(std::ostream& os,
                               IndentedLine& lineIndent) const {
        store.printUsageHelp(os, lineIndent);
    }
    using Flag<OnParseTrigger>::printUsageHelp;
};
template <typename OnParseFunc>
class ExclusiveFlagGroup : public FlagBase {
    ComplexFlag<OnParseFunc>& parentFlag;
    std::vector<FlagMap::iterator> flags;
    const std::string* _parsedValue;

   public:
    const std::string& parsedValue() { return *_parsedValue; }
    ExclusiveFlagGroup(ComplexFlag<OnParseFunc>& parentFlag, Policy policy)
        : FlagBase(policy, ""), parentFlag(parentFlag) {}

    inline virtual void parse(ArgIter&, ArgIter&) {
        std::cerr << "This should never be called\n";
        abort();
    }

    inline virtual void printUsageHelp(std::ostream& os,
                                       IndentedLine& lineIndent) const {
        AutoArgParse::printUsageHelp(flags, os, lineIndent);
    }
    inline virtual void printUsageSummary(std::ostream& os) const {
        if (flags.empty()) {
            return;
        }
        bool first = true;
        for (const auto& flagIter : flags) {
            if (first) {
                first = false;
            } else {
                os << "|";
            }
            os << flagIter->first;
            flagIter->second->printUsageSummary(os);
        }
    };

    /**
     * Wrapper around OnParseTriggers, insures exclusivity before  allowing a
     * successful parse
     */
    template <typename OnParseTriggerType>
    struct ExclusiveWrapper {
        ExclusiveFlagGroup<OnParseFunc>* parentGroup;
        OnParseTriggerType onParseSuccess;
        ExclusiveWrapper(ExclusiveFlagGroup<OnParseFunc>* parentGroup,
                         OnParseTriggerType&& onParseSuccess)
            : parentGroup(parentGroup), onParseSuccess(onParseSuccess) {}

        void operator()(const std::string& flag) {
            if (parentGroup->_parsed) {
                throwMoreThanOneExclusiveArgException(
                    parentGroup->parsedValue(), flag, parentGroup->flags);
            }
            parentGroup->_parsed = true;
            parentGroup->_available = false;
            parentGroup->_parsedValue = &flag;
            onParseSuccess(flag);
        }
    };

    template <template <class T> class FlagType,
              typename OnParseTriggerType = DefaultDoNothingHandler>
    FlagType<ExclusiveWrapper<OnParseTriggerType>>& add(
        const std::string& flag, const std::string& description,
        OnParseTriggerType&& trigger = DefaultDoNothingHandler()) {
        typedef ExclusiveWrapper<OnParseTriggerType> ExclusiveEnforcer;
        auto& flagObj = parentFlag.template add<FlagType>(
            flag, policy, description,
            ExclusiveEnforcer(this, std::forward<OnParseTriggerType>(trigger)));
        flags.push_back(std::move(parentFlag.store.flagInsertionOrder.back()));
        parentFlag.store.flagInsertionOrder.pop_back();
        if (flags.size() > 1) {
            if (policy == Policy::MANDATORY) {
                --parentFlag.store._numberMandatoryFlags;
            } else {
                --parentFlag.store._numberOptionalFlags;
            }
        }
        return *(static_cast<FlagType<ExclusiveEnforcer>*>(
            flags.back()->second.get()));
    }
    virtual inline bool isExclusiveGroup() { return true; }

    virtual std::vector<FlagMap::iterator>& getFlags() { return flags; }
    /**indevelopment
        template <template <class T> class FlagType, typename... StringFlags>
        auto with(StringFlags&&... flags)  -> decltype(){
            return std::tie(add<FlagType>(flags,"")...);
        }*/
};

template <typename T>
inline ExclusiveFlagGroup<T>& ComplexFlag<T>::makeExclusiveGroup(
    Policy policy) {
    auto flagIter = store.flags.insert(std::make_pair(
        std::to_string(store.flags.size()) + RANDOM_STRING,
        std::unique_ptr<FlagBase>(new ExclusiveFlagGroup<T>(*this, policy))));
    store.flagInsertionOrder.push_back(std::move(flagIter.first));
    return *(static_cast<ExclusiveFlagGroup<T>*>(
        store.flagInsertionOrder.back()->second.get()));
}
}
#endif /* AUTOARGPARSE_FLAGS_H_ */
