#ifndef AUTOARGPARSE_ARGPARSERBASE_H_
#define AUTOARGPARSE_ARGPARSERBASE_H_
#include <iostream>
#include <string>
#include <vector>
#include "indentedLine.h"
namespace AutoArgParse {
typedef std::vector<std::string>::iterator ArgIter;

enum Policy { MANDATORY, OPTIONAL };

// absolute base class for flags and args
class ParseToken {
   protected:
    bool _parsed;
    bool _available = true;

   public:
    const Policy policy;  // optional or mandatory
    const std::string
        description;  // help info on this parse token (e.g. flag, arg,etc.)

    ParseToken(const Policy policy, const std::string& description)
        : _parsed(false), policy(policy), description(description) {}
    virtual ~ParseToken() = default;

    inline bool available() { return _available; }
    /**
     * Return whether or not this parse token (arg/flag/etc.) was successfully
     * parsed.
     */
    inline bool parsed() const { return _parsed; }

    inline operator bool() const { return parsed(); }
};

/**Forward declaration of FlagStore such that it may be a friend */
class FlagStore;
class ArgBase : public ParseToken {
    friend FlagStore;

   protected:
    virtual void parse(ArgIter& first, ArgIter& last) = 0;

   public:
    const std::string name;  // name/description of the arg, not the argitself
    ArgBase(const std::string& name, const Policy policy,
            const std::string& description)
        : ParseToken(policy, description), name(name) {}
    virtual ~ArgBase() = default;
};

/**
 * non templated base of flag.
 */
class FlagBase : public ParseToken {
   public:
    virtual void parse(ArgIter& first, ArgIter& last) = 0;
    using ParseToken::ParseToken;
    inline virtual void printUsageHelp(std::ostream&, IndentedLine&) const {}
    inline virtual void printUsageSummary(std::ostream&) const {}

    inline void printUsageHelp(std::ostream& os) const {
        IndentedLine lineIndent(0);
        printUsageHelp(os, lineIndent);
    }
};
}
#endif /* AUTOARGPARSE_ARGPARSERBASE_H_ */
