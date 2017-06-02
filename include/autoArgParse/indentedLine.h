#ifndef AUTOARGPARSE_INDENTEDLINE_H_
#define AUTOARGPARSE_INDENTEDLINE_H_
#include <iostream>
namespace AutoArgParse {
class IndentedLine {
   public:
    int indentLevel;
    IndentedLine(int level) : indentLevel(level) {}
    IndentedLine() : indentLevel(0) {}

    inline void forcePrintIndent(std::ostream& os) const {
        for (int i = 0; i < indentLevel; i++) {
            os << "    ";
        }
    }

    inline void forceSingleIndent(std::ostream& os) { os << "    "; }
    friend inline std::ostream& operator<<(std::ostream& os,
                                           const IndentedLine& indent) {
        os << "\n";
        indent.forcePrintIndent(os);
        return os;
    }
};
}
#endif /* AUTOARGPARSE_INDENTEDLINE_H_ */
