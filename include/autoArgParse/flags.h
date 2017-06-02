
#ifndef AUTOARGPARSE_FLAGS_H_
#define AUTOARGPARSE_FLAGS_H_
#include <deque>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "argHandlers.h"
#include "argParserBase.h"

namespace AutoArgParse {

/**
 * Represents a simple flag, does not have any nested flags or arguments.
 */
template <typename OnParseTrigger>
class Flag : public FlagBase {
    friend FlagStore;
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
};

typedef std::unordered_map<std::string, std::shared_ptr<bool>> ExclusiveMap;
typedef std::unique_ptr<FlagBase> FlagPtr;
typedef std::unordered_map<std::string, FlagPtr> FlagMap;
typedef std::vector<std::unique_ptr<ArgBase>> ArgVector;

/**
 * A non templated object that can hold most of the data belonging to templated
 * class ComplexFlag and implement some of its functions.  The purpose of this
 * class is to allow non templated functions to be defined in the CPP file
 * rather than having to inline the code in this file.
 */
class FlagStore {
   public:
    FlagMap flags;
    std::deque<FlagMap::iterator> flagInsertionOrder;
    ExclusiveMap exclusiveFlags;
    ArgVector args;
    int _numberMandatoryFlags = 0;
    int _numberOptionalFlags = 0;
    int _numberExclusiveMandatoryFlags = 0;
    int _numberExclusiveOptionalFlags = 1;  // must be 1
    int _numberMandatoryArgs = 0;
    int _numberOptionalArgs = 0;

    bool tryParseArg(ArgIter& first, ArgIter& last, Policy& foundArgPolicy);
    bool tryParseFlag(ArgIter& first, ArgIter& last, Policy& foundFlagPolicy);

    void parse(ArgIter& first, ArgIter& last);
    void invokePrintFlag(
        std::ostream& os,
        const std::pair<const std::string, FlagPtr>& flagMapping,
        std::unordered_set<std::string>& printed) const;
    void printUsageSummary(std::ostream& os) const;
    void printUsageHelp(std::ostream& os, IndentedLine& lineIndent) const;
};

template <typename OnParseTrigger>
class ComplexFlag : public Flag<OnParseTrigger> {
    FlagStore store;

   protected:
    virtual void parse(ArgIter& first, ArgIter& last) {
        store.parse(first, last);
        this->_parsed = true;
    }

    inline void addExclusive(const std::string& flag,
                             std::shared_ptr<bool>& sharedState) {
        store.exclusiveFlags.insert(std::make_pair(flag, sharedState));
        if (store.flags[flag]->policy == Policy::MANDATORY) {
            ++store._numberExclusiveMandatoryFlags;
        } else {
            ++store._numberExclusiveOptionalFlags;
        }
    }

    inline const FlagStore& getFlagStore() { return store; }

   public:
    using Flag<OnParseTrigger>::Flag;

    inline const ArgVector& getArgs() const { return store.args; }

    inline const FlagMap& getFlags() const { return store.flags; }

    inline const ExclusiveMap& getExclusiveFlags() const {
        return store.exclusiveFlags;
    }

    const std::deque<FlagMap::iterator>& getFlagInsertionOrder() const {
        return store.flagInsertionOrder;
    }

    inline int numberMandatoryArgs() { return store._numberMandatoryArgs; }

    inline int numberOptionalArgs() { return store._numberOptionalArgs; }

    inline int numberMandatoryFlags() {
        return store._numberMandatoryFlags -
               store._numberExclusiveMandatoryFlags;
    }

    inline int numberOptionalFlags() {
        return store._numberOptionalFlags - store._numberExclusiveOptionalFlags;
    }

    template <template <class T> class FlagType,
              typename OnParseTriggerType = DefaultDoNothingHandler>
    typename std::enable_if<
        std::is_base_of<FlagBase, FlagType<OnParseTriggerType>>::value,
        FlagType<OnParseTriggerType>&>::type
    add(const std::string& flag, const Policy policy,
        const std::string& description,
        OnParseTriggerType&& trigger = DefaultDoNothingHandler()) {
        auto added = store.flags.insert(std::make_pair(
            flag,
            std::unique_ptr<FlagBase>(new FlagType<OnParseTriggerType>(
                policy, description, std::forward<OnParseTrigger>(trigger)))));
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
              typename ConverterFunc =  Converter<typename ArgType::ValueType>,
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

    template <typename... Strings>
    std::shared_ptr<bool> makeExclusive(const Strings&... strings) {
        int previousNMandatoryExclusives = store._numberExclusiveMandatoryFlags;
        int previousNOptionalExclusives = store._numberExclusiveOptionalFlags;
        std::shared_ptr<bool> sharedState = std::make_shared<bool>(false);
        // code below is just to apply the same operation to every passed in arg
        int unpack[]{0, (addExclusive(strings, sharedState), 0)...};
        static_cast<void>(unpack);
        if (store._numberExclusiveMandatoryFlags !=
            previousNMandatoryExclusives) {
            --store._numberExclusiveMandatoryFlags;
        }
        if (store._numberExclusiveOptionalFlags !=
            previousNOptionalExclusives) {
            --store._numberExclusiveOptionalFlags;
        }
        return sharedState;
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
}
#endif /* AUTOARGPARSE_FLAGS_H_ */
