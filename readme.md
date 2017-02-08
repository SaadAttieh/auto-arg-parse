#Auto Arg Parse 
#Fast and easy C++ argument parser 

* Automatic validation of program arguments,
*  Automatic generation of usage instructions,
*  Typesafe conversion from string input to desired type.

##Features
*  Validation of mandatory and optional arguments,
*  Validation of mandatory and optional flags,
*  Arbitrary nesting, flags may themselves have mandatory and optional flags/arguments,
*  Exclusive constraint on mandatory/optional flags
* Type safe user defined conversions from string input to required type,
*  Built in conversion for ints, more to come soon


##Coming soon

*  Variable number of arguments,
*  More built in conversions (other number formats, file path->stream conversion, etc.)
*  Validation of user define constraints, separate from those specified in the type conversion.


#Example Usage

##Code

````c++
#include<iostream>
#include "autoArgParse/argParser.h"
using namespace std;
using namespace AutoArgParse;


ArgParser argParser;

//Let's add an optional flag -p for power
auto& powerFlag = argParser.add<ComplexFlag>("-p", Policy::OPTIONAL,
                  "Specify power output.");

//give -p a mandatory argument, an integer
auto& powerArg = powerFlag.add<Arg<int>>("number_watts", Policy::MANDATORY,
                 "An integer representing the number of watts.");

//Add mandatory flag speed
auto& speedFlag = argParser.add<ComplexFlag>("--speed", Policy::MANDATORY,
                  "Specify the speed limit.");

//Add three exclusive flags, fast, medium slow
//no need to provide descriptions, these are self explanatory
auto& slow =  speedFlag.add<Flag>("slow", Policy::MANDATORY, "");
auto& medium =  speedFlag.add<Flag>("medium", Policy::MANDATORY, "");

//demonstrating nesting, let's make fast have an optional flag
auto& fast =  speedFlag.add<ComplexFlag>("fast", Policy::MANDATORY,
              "If fast is selected, an option of ignoring safety rules is available.");
auto& noSafety = fast.add<Flag>("--no-safety", Policy::OPTIONAL, "");

auto forceExclusive  = speedFlag.makeExclusive("slow", "medium", "fast");

int main(const int argc, const char** argv) {
    argParser.validateArgs(argc, argv);
    //the above will print an error and exit if not all arguments were
//     successfully parsed
    //otherwise, testing flags is easy

    if (powerFlag) {
        int power = powerArg.get();
        //already converted to integer
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
````

##Output:

After each error, the full usage information is printed out.  This has only been shown once.

```
$./testProg -p 
Error: Missing mandatory argument(s).  Valid option(s) are:  number_watts
Successfully parsed:  ./testProg

Usage: ./testProg [-p number_watts]  --speed slow|medium|fast [--no-safety]   

Arguments:
    -p number_watts
    [optional] Specify power output.
            number_watts: An integer representing the number of watts.
        --speed slow|medium|fast [--no-safety]  
    Specify the speed limit.
            fast [--no-safety] 
        If fast is selected, an option of ignoring safety rules is available.
                    
$./testProg  -p f
Error: Could not parse argument: number_watts
Could not interpret "f" as an integer.

Successfully parsed:  ./testProg -p
...
                    
$./testProg  -p 5 -p 3
Error: Repeated flag: -p
Successfully parsed:  ./testProg -p 5
...
                    
$./testProg -p 3 
Error: Missing mandatory argument(s). valid option(s) are:  --speed
Successfully parsed:  ./testProg
...
                    
$./testProg --speed fast slow
Error: The following arguments are exclusive and may not be used in conjunction: fast|medium|slow
Successfully parsed:  ./testProg --speed
...
                    
$./testProg  --speed fast --no-safety -p 3
Accepted power output of 3 W
running fast.
safety turned off.

````

