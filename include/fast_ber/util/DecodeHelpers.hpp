#pragma once

#include <span>
#include "fast_ber/ber_types/Identifier.hpp"
#include "fast_ber/util/BerView.hpp"

namespace fast_ber
{

struct DecodeResult
{
    bool success;
};

template <typename T>
DecodeResult decode(std::span<const uint8_t> input, T& output) noexcept
{
    return output.decode(BerView(input));
}

template <typename T>
DecodeResult decode(BerView input, T& output) noexcept
{
    return output.decode(input);
}

template <typename T>
DecodeResult decode(BerViewIterator& input, T& output) noexcept
{
    DecodeResult res = output.decode(*input);
    ++input;
    return res;
}

template <Class class_, Tag tag>
bool has_correct_header(BerView input, Id<class_, tag>, Construction construction)
{
    return input.is_valid() && input.class_() == class_ && input.tag() == tag && input.construction() == construction;
}

template <typename Identifier1, typename Identifier2>
bool has_correct_header(BerView input, DoubleId<Identifier1, Identifier2>, Construction construction)
{
    return input.is_valid() && input.identifier() == Identifier1{} &&
           input.construction() == Construction::constructed && input.begin()->is_valid() &&
           input.begin()->identifier() == Identifier2{} && input.begin()->construction() == construction;
}

} // namespace fast_ber
