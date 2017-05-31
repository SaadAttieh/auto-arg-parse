# Auto Arg Parse 
# Fast and easy C++ argument parser 

* Automatic validation of program arguments,
*  Automatic generation of usage instructions,
*  Type safe conversion from string input to desired type.

## Features
*  Validation of mandatory and optional arguments,
*  Validation of mandatory and optional flags,
*  Arbitrary nesting, flags may themselves have mandatory and optional flags or arguments,
* Type safe, (built in and  user defined) conversions from string input to required type,
*  Add constraints to arguments,
*  Make a set of flags exclusive,


## Coming soon

*  Variable number of arguments,
*  More built in conversions (other number formats, file path->stream conversion, etc.)


#Example Usage

## Setting up:
### Code:

```c++
#include <iostream>
#include "autoArgParse/argParser.h"
using namespace std;
using namespace AutoArgParse;

ArgParser argParser;
//declare flags and args here

int main(const int argc, const char** argv) {
//or here 
//then 
    argParser.validateArgs(argc, argv);
}
```

### Output:
If all flags and args are validated, proceed as normal, otherwise,an error is printed with the usage information (see below).

## Mandatory flag
### Code:
```c++
// Let's Add a mandatory flag speed
auto& speedFlag = argParser.add<Flag>("--speed", Policy::MANDATORY,
                                             "Specify the speed limit.");
```
### Output:

```
$./testProg 
Error: Missing mandatory argument(s). valid option(s) are:  --speed
Successfully parsed:  ./testProg
...
```

## Optional flag 
### Code:
````c++
// Let's add an optional flag -p for power
auto& powerFlag =
    argParser.add<Flag>("-p", Policy::OPTIONAL, "Use power.");
```

### Output:
```
$./testProg  -a
Error: Unexpected argument: -a
Valid option(s):  [-p]
...
$./testProg 
okay, -p is optional
$./testProg  -p
Okay, -p is optional and has been used
```

## Nesting, flags may themselves take flags and arguments
Give -p a mandatory integer argument, type safe conversion from string to int will automatically be used.

### Code:
```c++
// Let's add an optional flag -p for power.
//This time, -p is of type ComplexFlag.
//Allows nesting
auto& powerFlag =
    argParser.add<ComplexFlag>("-p", Policy::OPTIONAL, "Specify power output.");

// give -p a mandatory argument, an integer. Type safe conversion will be used
auto& powerArg =
    powerFlag.add<Arg<int>>("number_watts", Policy::MANDATORY,
                            "An integer representing the number of watts.");

```

### Output:
```
$./testProg -p 
Error: Missing mandatory argument(s).  Valid option(s) are:  number_watts
Successfully parsed:  ./testProg -p
...
$./testProg -p fudge
Error: Could not parse argument: number_watts
Could not interpret "fudge" as an integer.
...
```

## Add constraint to argument:
Power must be in range 0-50 inclusive.

## Code: modified from above
```c++

// Let's add an optional flag -p for power
auto& powerFlag =
    argParser.add<ComplexFlag>("-p", Policy::OPTIONAL, "Specify power output.");

// give -p a mandatory argument, an integer. Typesafe conversion will be used
auto& powerArg = powerFlag.add<Arg<int>>(
    "number_watts", Policy::MANDATORY,
    "An integer representing the number of watts in the range 0..50.",
    chain(Converter<int>(), IntRange(0, 50, true, true)));
```

### Output:
```
$./testProg -p 55 
Error: Could not parse argument: number_watts
Expected value to be between 0(inclusive) and 50(inclusive).
...
```

## Exclusive flags:

### Code:
```c++
auto& speedFlag = argParser.add<ComplexFlag>("--speed", Policy::MANDATORY,
                                             "Specify the speed.");

// Add three exclusive flags, fast, medium slow
// no need to provide descriptions, these are self explanatory
auto& slow = speedFlag.add<Flag>("slow", Policy::MANDATORY, "");
auto& medium = speedFlag.add<Flag>("medium", Policy::MANDATORY, "");
auto& fast = speedFlag.add<Flag>("fast", Policy::MANDATORY, "");
auto forceExclusive = speedFlag.makeExclusive("slow", "medium", "fast");
```

### Output:
```
$ ./testProg --speed 
Error: Missing mandatory argument(s). valid option(s) are:  slow, medium, fast
Successfully parsed:  ./testProg --speed
...
$ ./testProg --speed fast slow
Error: The following arguments are exclusive and may not be used in conjunction: fast|medium|slow
Successfully parsed:  ./testProg --speed
...
```

## Usage information:
### Code:
If an error is reported, the usage information is printed out.  Otherwise, the information can be manually printed:
```c++
int main(const int argc, const char** argv) {
    argParser.printAllUsageInfo(cout, argv[0]);
}
```

### Output:
Taken from code src/exampleUsage.cpp
```
Usage: ./testProg [-p number_watts]  --speed slow|medium|fast  

Arguments:
    -p number_watts
    [optional] Specify power output.
            number_watts: An integer representing the number of watts.
        --speed slow|medium|fast 
    Specify the speed.
            
```

## Testing optional flags after validation:
### Code:
All flags and arguments are implicitly convertible to bool (true=parsed, false otherwise).  This can be used to test whether an optional flag has been parsed.
```c++
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
    }
}

```


# Implementation:
auto-arg-parse is designed with the aim of allowing a __correct__ input to be validated as quick as possible.  This has the cost of making the process of printing errors/usage instructions less efficient.  More specifically, some data structures which may have sped up the printing of usage instructions (e.g. additional sets/maps) have not been used as these would have slowed down the construction of the argument parser.  However, since the usual procedure after detecting an invalid set of arguments is to exit the program, it was decided that fast processing of valid input was more important.

This section TBC