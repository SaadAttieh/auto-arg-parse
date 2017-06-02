
#ifndef AUTOARGPARSE_ARGS_H_
#define AUTOARGPARSE_ARGS_H_
#include "argParserBase.h"
namespace AutoArgParse {

// forward decls of functions that throw exceptions, cannot include
// parseException.h in this file due to circular dependencies
void throwFailedArgConversionException(const std::string& name,
                                       const std::string& additionalExpl);

template <typename T, typename ConverterFunc = DefaultDoNothingHandler>
class Arg : public ArgBase {
   public:
    typedef T ValueType;

   private:
    T parsedValue;
    ConverterFunc convert;

   protected:
    virtual inline void parse(ArgIter& first, ArgIter&) {
        _parsed = false;
        try {
            convert(*first, parsedValue);
            ++first;
            _parsed = true;
        } catch (ErrorMessage& e) {
            if (this->policy == Policy::MANDATORY) {
                throwFailedArgConversionException(this->name, e.message);
            } else {
                return;
            }
        }
    }

   public:
    Arg(const std::string& name, const Policy policy,
        const std::string& description, ConverterFunc convert)
        : ArgBase(name, policy, description), convert(std::move(convert)) {}

    T& get() { return parsedValue; }
};
}

#endif /* AUTOARGPARSE_ARGS_H_ */
