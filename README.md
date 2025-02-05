# fast_ber ![version](https://img.shields.io/github/tag/samuel-tyler/fast_ber.svg) [![Appveyor status](https://ci.appveyor.com/api/projects/status/github/Samuel-Tyler/fast_ber?branch=master&svg=true)](https://ci.appveyor.com/project/Samuel-Tyler/fast-ber) [![Travis status](https://api.travis-ci.com/Samuel-Tyler/fast_ber.svg?branch=master)](https://travis-ci.com/Samuel-Tyler/fast_ber) ![C++11](https://img.shields.io/badge/language-C%2B%2B11-green.svg) ![C++14](https://img.shields.io/badge/language-C%2B%2B14-green.svg) ![C++17](https://img.shields.io/badge/language-C%2B%2B17-green.svg) ![C++20](https://img.shields.io/badge/language-C%2B%2B20-green.svg) ![clang-format](https://github.com/Samuel-Tyler/fast_ber/workflows/clang-format/badge.svg)
A performant ASN.1 BER encoding and decoding library written in C++11

## Introduction
fast_ber is a small, lightweight library for BER encoding and decoding. Fast ber forgoes some tight ASN.1 specification conformance to provide fast encoding and decoding performance in common use cases

#### Design Decisions
- Simple, modern C++ interface
- ASN.1 sequences are represented as POD structs - no private members or complex getters and setters
- No exceptions, no RTTI and limited memory allocations (everything is small buffer optimised)
- View classes are provided for zero copy decoding
- Interfaces mimic STL types such as std::string, std::vector and std::optional

[Read more about the decisions made writing this library](https://medium.com/@samtyler6/translating-asn-1-ber-types-to-modern-c-226fb06d026f)

#### Limitations
- No circular data structures
- Size and value constraints are not implemented

### Tools
fast_ber_view can be used to dump the contents of a BER PDU, without requiring a schema.
```
./build/src/fast_ber_view ./build_gcc/sample/pokemon.ber | jq
{
  "length": 125,
  "identifier": {
    "class": "Universal",
    "tag": "Sequence / Sequence Of"
  },
  "content": ...
}
```

## Call for Test Data
Test data is wanted to improve this project! If you have any test ASN.1 specs or BER files please share them. More test data will improve parsing and help find any issues with the library.

## Example Project
`fast_ber` is designed to be easy to consume via CMake. This is demonstrated in [`fast_ber_ldap3`](https://github.com/Samuel-Tyler/fast_ber_ldap3), an example project using `fast_ber` to create and dissect LDAP3 certificates.

## Compiler Usage
1. Build the compiler:
```
git submodule update --init
mkdir build_cmake
cd build_cmake
cmake ..
make
ctest
```

2. Define an ASN.1 file - Example: pokemon.asn
```
Pokemon DEFINITIONS EXPLICIT TAGS ::= BEGIN

Team ::= SEQUENCE {
    team-name OCTET STRING,
    members SEQUENCE OF Pokemon
}

Pokemon ::= SEQUENCE {
    name OCTET STRING,
    category OCTET STRING,
    type Type,
    ability OCTET STRING,
    weakness OCTET STRING,
    weight INTEGER
}

Type ::= ENUMERATED {
    normal,
    fire,
    fighting,
    water,
    flying,
    grass
}

END
```

3. Compile an asn file into a header file defining the ber structure
```
cd build_cmake
./src/fast_ber_compiler pokemon.asn pokemon
```
output:
```
#pragma once

#include "fast_ber/ber_types/All.hpp"
#include "pokemon.fwd.hpp"


namespace fast_ber {
using namespace abbreviations;

namespace Pokemon {

enum class TypeValues {
    normal,
    fire,
    fighting,
    water,
    flying,
    grass,
};

template <typename Identifier>
FAST_BER_ALIAS(Type, Enumerated<TypeValues>);

template <typename Identifier>
struct Pokemon {
    OctetString<> name;
    OctetString<> category;
    Type<> type;
    OctetString<> ability;
    OctetString<> weakness;
    Integer<> weight;
};

template <typename Identifier>
struct Team {
    OctetString<> team_name;
    SequenceOf<Pokemon<>> members;
};

} // End namespace Pokemon
} // End namespace fast_ber
```

4. Include the header file into your application. Fields in the generated structure can be assigned to standard types. Encode and decode functions are used to serialize and deserialize the data
```
#include "pokemon.hpp"

int main()
{
    fast_ber::Pokemon::Team<>    team    = {"Sam's Team"};
    fast_ber::Pokemon::Pokemon<> muchlax = { "Munchlax",
                                             "Big Eater",
                                             fast_ber::Pokemon::Type<>::Values::normal,
                                             "Thick Fat, Pickup",
                                             "Fighting",
                                             105 };
    fast_ber::Pokemon::Pokemon<> piplup  = { "Piplup",
                                             "Penguin",
                                             fast_ber::Pokemon::Type<>::Values::water,
                                             Torrent",
                                             "Electric, Grass",
                                             12 };
    team.members.push_back(muchlax);
    team.members.push_back(piplup);

    std::array<uint8_t, 2000> buffer{};
    const auto                encode_result = fast_ber::encode(std::span<uint8_t>(buffer), team);
    if (!encode_result.success)
    {
        return -1;
    }

    return 0;
}
```

Take a look at [`fast_ber_ldap3`](https://github.com/Samuel-Tyler/fast_ber_ldap3) for an example of these steps in action.

## Features
fast_ber is in development. The following table documents what has been implemented and what features are coming soon

| Type | Implemented |
| --- | --- |
| Integer | Yes |
| Boolean | Yes |
| Strings | Yes |
| Sequence | Yes |
| Sequence Of | Yes |
| Choice | Yes |
| Set | No |
| Set Of | Yes |
| Enumerated | Yes |
| Object Identifier | Yes |
| Dates / Times | No |
| Null | Yes |
| Any | No |

| Feature | Implemented |
| --- | --- |
| Explicit tags | Yes |
| Implicit tags | Yes |
| Indefinitive length | No |
| Arbitrary top level structure | Yes |
| Arbritary definition order | Yes |
| Embedded Types | Yes |
| Imports | Yes |

## Possible Future Extensions
The following features may be added in the future

```
Partial decoding
Partial encode at compile time (templated / constexpr)
High / low conformance modes
DER mode
```

## Benchmarks

fast_ber includes benchmarks against asn1c, an ASN library written in C. Here is an example of the output:
```
-------------------------------------------------------------------------------
Benchmark: Decode Performance
-------------------------------------------------------------------------------
...............................................................................

benchmark name                                  iters   elapsed ns      average
-------------------------------------------------------------------------------
fast_ber        - 1,000,000 x decode 2107B pdu      1    516381442   516.381 ms
asn1c           - 1,000,000 x decode 2107B pdu      1   4996255249    4.99626 s
fast_ber        - 1,000,000 x decode 64B pdu        1    192230063    192.23 ms
asn1c           - 1,000,000 x decode 64B pdu        1   2581069031    2.58107 s

-------------------------------------------------------------------------------
Benchmark: Encode Performance
-------------------------------------------------------------------------------
...............................................................................

benchmark name                                  iters   elapsed ns      average
-------------------------------------------------------------------------------
fast_ber        - 1,000,000 x encode 2107B pdu      1    191266512   191.267 ms
asn1c           - 1,000,000 x encode 2107B pdu      1   7349946740    7.34995 s

-------------------------------------------------------------------------------
Benchmark: Object Construction Performance
-------------------------------------------------------------------------------
...............................................................................

benchmark name                                  iters   elapsed ns      average
-------------------------------------------------------------------------------
fast_ber        - 1,000,000 x construct data        1   1005938231    1.00594 s
asn1c           - 1,000,000 x construct data        1    511881940   511.882 ms

-------------------------------------------------------------------------------
Benchmark: Calculate Encoded Length Performance
-------------------------------------------------------------------------------
...............................................................................

benchmark name                                  iters   elapsed ns      average
-------------------------------------------------------------------------------
fast_ber        - 1,000,000 x encoded length        1     17084558   17.0846 ms

===============================================================================
All tests passed (31 assertions in 8 test cases)
```
