﻿#include "autogen/parameterized_types.hpp"

#include "catch2/catch.hpp"

#include <array>

TEST_CASE("Parameterized Types: Encode and decode parameterized types")
{
    REQUIRE(std::is_same<fast_ber::Parameterized::T1::AliasedType, decltype(fast_ber::Parameterized::T3::b)>::value);
    REQUIRE(std::is_same<fast_ber::Integer<>, decltype(fast_ber::Parameterized::T4::A::b)>::value);
    REQUIRE(std::is_same<fast_ber::OctetString<>, decltype(fast_ber::Parameterized::T4::B::b)>::value);

    std::array<uint8_t, 100>    buffer = {};
    fast_ber::Parameterized::T3 one    = {5, "param"};
    fast_ber::Parameterized::T3 two    = {10, ";"};

    one.a = 5;
    one.b = "param";

    fast_ber::EncodeResult encode_result = fast_ber::encode(std::span(buffer.data(), buffer.size()), one);
    fast_ber::DecodeResult decode_result = fast_ber::decode(std::span(buffer.data(), buffer.size()), two);

    REQUIRE(encode_result.success);
    REQUIRE(decode_result.success);

    REQUIRE(one == two);
    REQUIRE(two.a == 5);
    REQUIRE(two.b == "param");
}
