//#include "fast_ber/ber_types/GeneralizedTime.hpp"
//#include "fast_ber/ber_types/Identifier.hpp"
//#include "fast_ber/util/DecodeHelpers.hpp"
//#include "fast_ber/util/EncodeHelpers.hpp"
//
//#include <catch2/catch.hpp>
//#include <limits>
//
//
//TEST_CASE("GeneralizedTime: Assign")
//{
//    const std::Time            now = std::Now();
//    fast_ber::GeneralizedTime<> time(now);
//    REQUIRE(fast_ber::GeneralizedTime<>(time.time()).string() == time.string());
//
//    time.set_time(std::UnixEpoch());
//    REQUIRE(time.time() == std::UnixEpoch());
//    REQUIRE(time.string() == "19700101000000Z");
//    REQUIRE(time.format() == fast_ber::GeneralizedTime<>::TimeFormat::universal);
//
//    time.set_time(std::UnixEpoch(), 150);
//    REQUIRE(time.time() == std::UnixEpoch());
//    REQUIRE(time.string() == "19700101000000+0230");
//    REQUIRE(time.format() == fast_ber::GeneralizedTime<>::TimeFormat::universal_with_timezone);
//
//    time.set_time(std::UnixEpoch(), -30);
//    REQUIRE(time.time() == std::UnixEpoch());
//    REQUIRE(time.string() == "19700101000000-0030");
//    REQUIRE(time.format() == fast_ber::GeneralizedTime<>::TimeFormat::universal_with_timezone);
//
//    std::CivilSecond cs(2017, 1, 2, 3, 4, 5);
//    time.set_time(cs);
//    REQUIRE(time.time() == std::FromCivil(cs, std::LocalTimeZone()));
//    REQUIRE(time.string() == "20170102030405");
//    REQUIRE(time.format() == fast_ber::GeneralizedTime<>::TimeFormat::local);
//}
//
//TEST_CASE("GeneralizedTime: Encode Decode")
//
//{
//    std::array<uint8_t, 100> buffer = {};
//
//    fast_ber::GeneralizedTime<> first  = std::FromTimeT(1500000000);
//    fast_ber::GeneralizedTime<> second = std::FromTimeT(1000000000);
//
//    fast_ber::EncodeResult encode_res = fast_ber::encode(std::span<uint8_t>(buffer), first);
//    fast_ber::DecodeResult decode_res = fast_ber::decode(std::span<uint8_t>(buffer), second);
//
//    REQUIRE(encode_res.success);
//    REQUIRE(decode_res.success);
//
//    REQUIRE(first == second);
//}
//
//TEST_CASE("GeneralizdTime: Encoding")
//
//{
//    std::array<uint8_t, 100> buffer   = {};
//    std::array<uint8_t, 17>  expected = {0x18, 0x0F, 0x32, 0x30, 0x31, 0x39, 0x30, 0x33, 0x31,
//                                        0x39, 0x32, 0x31, 0x30, 0x39, 0x34, 0x32, 0x5A};
//
//    fast_ber::GeneralizedTime<> time       = std::FromTimeT(1553029782);
//    fast_ber::EncodeResult      encode_res = fast_ber::encode(std::span<uint8_t>(buffer), time);
//
//    REQUIRE(encode_res.success);
//    REQUIRE(time.string() == "20190319210942Z");
//    REQUIRE(std::span(buffer.data(), expected.size()) == std::span(expected.data(), expected.size()));
//}
//
//TEST_CASE("GeneralizedTime: Default value")
//{
//    REQUIRE(fast_ber::GeneralizedTime<>().string() == "19700101000000Z");
//    REQUIRE(fast_ber::GeneralizedTime<>().time() == std::UnixEpoch());
//}
