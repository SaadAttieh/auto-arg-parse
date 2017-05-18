#include <iostream>
#include "autoArgParse/argParser.h"
using namespace std;
using namespace AutoArgParse;

ArgParser argParser;

// Let's add an optional flag -p for power
auto& powerFlag =
    argParser.add<ComplexFlag>("-p", Policy::OPTIONAL, "Specify power output.");

// give -p a mandatory argument, an integer. Typesafe conversion will be used
auto& powerArg =
    powerFlag.add<Arg<int>>("number_watts", Policy::MANDATORY,
                            "An integer representing the number of watts.");

// Let's Add a mandatory flag speed
auto& speedFlag = argParser.add<ComplexFlag>("--speed", Policy::MANDATORY,
                                             "Specify the speed limit.");

// Add three exclusive flags, fast, medium slow
// no need to provide descriptions, these are self explanatory
auto& slow = speedFlag.add<Flag>("slow", Policy::MANDATORY, "");
auto& medium = speedFlag.add<Flag>("medium", Policy::MANDATORY, "");
auto& fast = speedFlag.add<ComplexFlag>(
    "fast", Policy::MANDATORY,
    "If fast is selected, an option of ignoring safety rules is available.");
// fast is a complex flag to allow further nesting, see below
auto forceExclusive = speedFlag.makeExclusive("slow", "medium", "fast");
// let's add a nested flag, a limit to how fast we can go
// We wwill use a constraint to insure the integer is within the correct range
auto& fastLimit =
    fast.add<ComplexFlag>("--limit", Policy::OPTIONAL, "Limit the top speed.");
IntRange range(0, 50, true, true);
auto& fastLimitValue = fastLimit.add<Arg<int>>("speed_limit", Policy::MANDATORY,
                                               "An integer with range 0..50",
                                               Converter<int>(), range);

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
        if (fastLimitValue) {
            cout << "Speed limit is " << fastLimitValue.get() << endl;
        }
    }
}
