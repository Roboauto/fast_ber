﻿#pragma once

#include "fast_ber/util/BerView.hpp"
#include "fast_ber/util/DecodeHelpers.hpp"
#include "fast_ber/util/EncodeHelpers.hpp"
#include "fast_ber/util/EncodeIdentifiers.hpp"

#include <vector>
#include <span>

#include <array>

namespace fast_ber
{

enum class ConstructionMethod
{
    construct_with_provided_content,
    construct_from_ber_packet
};

// Owning container of a ber packet. Contents may or may not be valid. Memory is stored in a small buffer optimized
// vector. Can be constructed directly from encoded ber memory (decode) or by specifiyng the desired contents
// (assign_contents)
class BerContainer
{
  public:
    BerContainer() noexcept;
    BerContainer(const BerView input_view) noexcept { decode(input_view); }
    BerContainer(const BerContainer& container) noexcept { decode(container.view()); }
    BerContainer(BerContainer&& container) noexcept { decode(container.view()); }
    BerContainer(std::span<const uint8_t> input_data, ConstructionMethod method) noexcept;
    BerContainer(Construction input_construction, Class input_class, Tag input_tag,
                 std::span<const uint8_t> input_content) noexcept;
    ~BerContainer() noexcept = default;

    BerContainer& operator=(const BerView input_view) noexcept;
    BerContainer& operator=(const BerContainer& input_container) noexcept;
    BerContainer& operator=(BerContainer&& input_container) noexcept;

    void assign_content(const std::span<const uint8_t> input_content) noexcept;
    void assign_content(Construction input_construction, Class input_class, Tag input_tag,
                        std::span<const uint8_t> input_content) noexcept;
    void resize_content(size_t size);

    bool         is_valid() const noexcept { return m_view.is_valid(); }
    Construction construction() const noexcept { return m_view.construction(); }
    Class        class_() const noexcept { return m_view.class_(); }
    Tag          tag() const noexcept { return m_view.tag(); }
    size_t       identifier_length() const noexcept { return m_view.identifier_length(); }
    size_t       header_length() const noexcept { return m_view.header_length(); }

    std::span<uint8_t>       content() noexcept { return m_view.content(); }
    std::span<const uint8_t> content() const noexcept { return m_view.content(); }
    uint8_t*                  content_data() noexcept { return m_view.content_data(); }
    const uint8_t*            content_data() const noexcept { return m_view.content_data(); }
    size_t                    content_length() const noexcept { return m_view.content_length(); }

    std::span<const uint8_t> ber() const noexcept { return m_view.ber(); }
    const uint8_t*            ber_data() const noexcept { return m_view.ber_data(); }
    size_t                    ber_length() const noexcept { return m_view.ber_length(); }

    const BerView view() const noexcept { return m_view; }

    EncodeResult encode(std::span<uint8_t> buffer) const noexcept;
    DecodeResult decode(const BerView input_data) noexcept;

  private:
    std::vector<uint8_t> m_data;
    MutableBerView                     m_view;
};

inline BerContainer::BerContainer() noexcept
    : m_data{0x00, 0x00}, m_view(std::span(m_data.data(), m_data.size()), 0, 2, 0)
{
}

inline BerContainer::BerContainer(std::span<const uint8_t> input_data, ConstructionMethod method) noexcept
    : m_data(input_data.begin(), input_data.end()), m_view(std::span(m_data.data(), m_data.size()))
{
    if (method == ConstructionMethod::construct_with_provided_content)
    {
        assign_content(input_data);
    }
    else if (method == ConstructionMethod::construct_from_ber_packet)
    {
        decode(input_data);
    }
    else
    {
        assert(0);
    }
}

inline BerContainer::BerContainer(Construction input_construction, Class input_class, Tag input_tag,
                                  std::span<const uint8_t> input_content) noexcept
{
    assign_content(input_construction, input_class, input_tag, input_content);
}

inline BerContainer& BerContainer::operator=(const BerContainer& container) noexcept
{
    m_data = container.m_data;
    m_view.assign(m_data, container.tag(), container.header_length(), container.content_length());
    return *this;
}

inline BerContainer& BerContainer::operator=(BerContainer&& container) noexcept
{
    m_data = std::move(container.m_data);
    m_view.assign(m_data, container.tag(), container.header_length(), container.content_length());
    return *this;
}

inline void BerContainer::assign_content(const std::span<const uint8_t> input_content) noexcept
{
    m_data.resize(15 + input_content.size());
    m_data[0] = 0x00; // No identifier provided, use a tag of 0
    size_t new_header_length =
        1 + encode_length(std::span<uint8_t>(m_data.data() + 1, m_data.size() - 1), input_content.size());

    m_data.resize(new_header_length + input_content.size());
    std::copy(input_content.begin(), input_content.end(), m_data.begin() + new_header_length);
    m_view.assign(m_data, 0, new_header_length, input_content.size());

    assert(m_view.is_valid());
}

inline void BerContainer::assign_content(Construction input_construction, Class input_class, Tag input_tag,
                                         std::span<const uint8_t> input_content) noexcept
{
    m_data.resize(30 + input_content.size());
    size_t input_header_length = encode_header(std::span(m_data.data(), m_data.size()), input_construction,
                                               input_class, input_tag, input_content.size());

    m_data.resize(input_header_length + input_content.size());
    std::copy(input_content.begin(), input_content.end(), m_data.begin() + input_header_length);
    m_view.assign(m_data, input_header_length, input_content.size());

    assert(m_view.is_valid());
}

inline void BerContainer::resize_content(size_t size)
{
    std::array<uint8_t, 20> length_buffer{};

    size_t old_header_length   = view().header_length();
    size_t old_size            = view().content_length();
    size_t new_length_offset   = view().identifier_length();
    size_t new_length_length   = encode_length(std::span(length_buffer), size);
    size_t new_header_length   = new_length_offset + new_length_length;
    size_t new_complete_length = new_header_length + size;

    m_data.resize(new_complete_length);
    std::memmove(m_data.data() + new_header_length, m_data.data() + old_header_length, std::min(old_size, size));
    std::copy(length_buffer.data(), length_buffer.data() + new_length_length, m_data.data() + new_length_offset);
    m_view.assign(std::span(m_data.data(), m_data.size()), new_header_length, size);

    assert(new_length_length != 0);
    assert(m_view.is_valid());
}

inline EncodeResult BerContainer::encode(std::span<uint8_t> buffer) const noexcept { return m_view.encode(buffer); }

inline DecodeResult BerContainer::decode(const BerView input_view) noexcept
{
    if (!input_view.is_valid())
    {
        m_data = {};
        m_view.assign(std::span<uint8_t>(m_data));
        assert(!m_view.is_valid());
        return DecodeResult{false};
    }

    m_data.assign(input_view.ber().begin(), input_view.ber().end());
    m_view.assign(std::span(m_data.data(), m_data.size()), input_view.tag(), input_view.header_length(),
                  input_view.content_length());

    return DecodeResult{true};
}

} // namespace fast_ber
