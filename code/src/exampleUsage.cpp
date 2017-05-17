#include <iostream>
#include "autoArgParse/argParser.h"
using namespace std;
using namespace AutoArgParse;

ArgParser argParser;

// Let's add an optional flag -p for power
auto& powerFlag =
    argParser.add<ComplexFlag>("-p", Policy::OPTIONAL, "Specify power output.");

// give -p a mandatory argument, an integer
auto& powerArg =
    powerFlag.add<Arg<int>>("number_watts", Policy::MANDATORY,
                            "An integer representing the number of watts.");

// Add mandatory flag speed
auto& speedFlag = argParser.add<ComplexFlag>("--speed", Policy::MANDATORY,
                                             "Specify the speed limit.");

// Add three exclusive flags, fast, medium slow
// no need to provide descriptions, these are self explanatory
auto& slow = speedFlag.add<Flag>("slow", Policy::MANDATORY, "");
auto& medium = speedFlag.add<Flag>("medium", Policy::MANDATORY, "");

// demonstrating nesting, let's make fast have an optional flag
auto& fast = speedFlag.add<ComplexFlag>(
    "fast", Policy::MANDATORY,
    "If fast is selected, an option of ignoring safety rules is available.");
auto& noSafety = fast.add<Flag>("--no-safety", Policy::OPTIONAL, "");

auto forceExclusive = speedFlag.makeExclusive("slow", "medium", "fast");

int main(const int argc, const char** argv) {
    argParser.validateArgs(argc, argv);
    // the above will print an error and exit if not all arguments were
    //     successfully parsed
    // otherwise, testing flags is easy

    if (powerFlag) {
        int power = powerArg.get();
        // already converted to integer
        cout << "Accepted power output of " << power << " W" << endl;
    }
    if (slow) {
        cout << "Running slowly." << endl;
    } else if (medium) {
        cout << "Running normally." << endl;
    } else if (fast) {
        cout << "running fast." << endl;
        if (noSafety) {
            cout << "safety turned off." << endl;
        }
    }
}