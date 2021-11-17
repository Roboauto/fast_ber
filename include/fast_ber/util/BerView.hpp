#pragma once

#include "fast_ber/ber_types/Class.hpp"
#include "fast_ber/ber_types/Construction.hpp"
#include "fast_ber/util/EncodeHelpers.hpp"
#include "fast_ber/util/EncodeIdentifiers.hpp"
#include "fast_ber/util/Extract.hpp"

#include <span>

#include <cstring>

namespace fast_ber
{

class BerViewIterator;
class MutableBerViewIterator;
enum class End
{
    end
};

class BerView
{
  public:
    constexpr BerView() noexcept = default;
    BerView(std::span<const uint8_t> input_ber_data) noexcept { assign(input_ber_data); }
    BerView(std::span<const uint8_t> input_ber_data, size_t input_header_length, size_t input_content_length) noexcept;
    BerView(std::span<const uint8_t> input_ber_data, Tag input_tag, size_t input_header_length,
            size_t content_length) noexcept;

    void assign(std::span<const uint8_t> input_ber_data) noexcept;
    void assign(std::span<const uint8_t> input_ber_data, size_t input_header_length,
                size_t input_content_length) noexcept;
    void assign(std::span<const uint8_t> input_ber_data, Tag input_tag, size_t input_header_length,
                size_t content_length) noexcept;

    bool         is_valid() const noexcept { return m_data != nullptr; }
    Construction construction() const noexcept { return get_construction(m_data[0]); }
    RuntimeId    identifier() const { return {class_(), tag()}; }
    Class        class_() const noexcept { return get_class(m_data[0]); }
    Tag          tag() const noexcept { return m_tag; }
    size_t       identifier_length() const noexcept { return encoded_tag_length(tag()); }
    size_t       header_length() const noexcept { return m_header_length; }

    std::span<const uint8_t> content() const noexcept { return std::span(content_data(), m_content_length); }
    const uint8_t*            content_data() const noexcept { return m_data + m_header_length; }
    size_t                    content_length() const noexcept { return m_content_length; }
    std::span<const uint8_t> ber() const noexcept { return std::span(ber_data(), ber_length()); }
    const uint8_t*            ber_data() const noexcept { return m_data; }
    size_t                    ber_length() const noexcept { return m_header_length + m_content_length; }

    BerViewIterator begin() const noexcept;
    BerViewIterator end() const noexcept;

    EncodeResult encode(std::span<uint8_t> buffer) const noexcept;

  private:
    const uint8_t* m_data           = nullptr;
    Tag            m_tag            = 0;
    size_t         m_header_length  = 0; // Also content offset
    size_t         m_content_length = 0;
};

class MutableBerView : public BerView
{
  public:
    MutableBerView() noexcept : BerView() {}
    MutableBerView(std::span<uint8_t> data) noexcept : BerView(data) {}
    MutableBerView(std::span<uint8_t> a, size_t b, size_t c) : BerView(a, b, c) {}
    MutableBerView(std::span<const uint8_t> a, Tag b, size_t c, size_t d) noexcept : BerView(a, b, c, d) {}

    BerViewIterator        cbegin() const noexcept;
    BerViewIterator        cend() const noexcept;
    MutableBerViewIterator begin() noexcept;
    MutableBerViewIterator end() noexcept;

    uint8_t* content_data() noexcept
    {
        return const_cast<uint8_t*>(BerView::content_data()); // NOLINT(cppcoreguidelines-pro-type-const-cast)
    }
    const uint8_t*            content_data() const noexcept { return BerView::content_data(); }
    std::span<uint8_t>       content() noexcept { return std::span(content_data(), BerView::content().size()); }
    std::span<const uint8_t> content() const noexcept { return BerView::content(); }
};

class BerViewIterator
{
  public:
    BerViewIterator(std::span<const uint8_t> buffer) noexcept : m_buffer(buffer), m_view(buffer) {}
    BerViewIterator(End) noexcept : m_buffer(), m_view() {}

    BerViewIterator& operator++() noexcept
    {
        if (!m_view.is_valid())
        {
            assert(m_view.ber_length() == 0);
        }
        m_buffer = m_buffer.subspan(m_view.ber_length());
        m_view.assign(m_buffer);

        return *this;
    }

    const BerView& operator*() const noexcept { return m_view; }
    const BerView* operator->() const noexcept { return &m_view; }

    friend bool operator==(const BerViewIterator& lhs, const BerViewIterator& rhs) noexcept
    {
        if (!lhs.m_view.is_valid() && !rhs.m_view.is_valid())
        {
            return true;
        }
        else
        {
            return lhs.m_buffer.data() == rhs.m_buffer.data();
        }
    }

    friend bool operator!=(const BerViewIterator& lhs, const BerViewIterator& rhs) noexcept { return !(lhs == rhs); }

    using difference_type   = std::ptrdiff_t;
    using value_type        = const BerView;
    using pointer           = const BerView*;
    using reference         = const BerView&;
    using iterator_category = std::forward_iterator_tag;

  private:
    std::span<const uint8_t> m_buffer;
    BerView                   m_view;
};

class MutableBerViewIterator
{
  public:
    MutableBerViewIterator(std::span<uint8_t> buffer) noexcept : m_buffer(buffer), m_view(buffer) {}
    MutableBerViewIterator(End) noexcept : m_buffer(), m_view() {}

    MutableBerViewIterator& operator++() noexcept
    {
        if (!m_view.is_valid())
        {
            assert(m_view.ber_length() == 0);
        }
        m_buffer = m_buffer.subspan(m_view.ber_length());
        m_view.assign(m_buffer);

        return *this;
    }

    MutableBerView& operator*() noexcept { return m_view; }
    MutableBerView* operator->() noexcept { return &m_view; }

    friend bool operator==(const MutableBerViewIterator& lhs, const MutableBerViewIterator& rhs) noexcept
    {
        if (!lhs.m_view.is_valid() && !rhs.m_view.is_valid())
        {
            return true;
        }
        else
        {
            return lhs.m_buffer.data() == rhs.m_buffer.data();
        }
    }

    friend bool operator!=(const MutableBerViewIterator& lhs, const MutableBerViewIterator& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    using difference_type   = std::ptrdiff_t;
    using value_type        = MutableBerView;
    using pointer           = MutableBerView*;
    using reference         = MutableBerView&;
    using iterator_category = std::forward_iterator_tag;

  private:
    std::span<uint8_t> m_buffer;
    MutableBerView      m_view;
};

inline BerView::BerView(std::span<const uint8_t> input_ber_data, size_t input_header_length,
                        size_t input_content_length) noexcept
{
    assign(input_ber_data, input_header_length, input_content_length);
}

inline BerView::BerView(std::span<const uint8_t> input_ber_data, Tag input_tag, size_t input_header_length,
                        size_t input_content_length) noexcept
{
    assign(input_ber_data, input_tag, input_header_length, input_content_length);
}

inline void BerView::assign(std::span<const uint8_t> input_ber_data) noexcept
{
    size_t input_content_length = 0;

    size_t input_tag_length      = extract_tag(input_ber_data, m_tag);
    size_t input_len_length      = extract_length(input_ber_data, input_content_length, input_tag_length);
    size_t input_header_length   = input_tag_length + input_len_length;
    size_t input_complete_length = input_header_length + input_content_length;

    if (input_tag_length == 0 || input_len_length == 0 || input_complete_length > input_ber_data.size())
    {
        m_header_length  = 0;
        m_content_length = 0;
        m_data           = nullptr;
        return;
    }

    m_data           = input_ber_data.data();
    m_header_length  = input_header_length;
    m_content_length = input_content_length;
}

inline void BerView::assign(std::span<const uint8_t> input_ber_data, size_t input_header_length,
                            size_t input_content_length) noexcept
{
    extract_tag(input_ber_data, m_tag);
    assign(input_ber_data, m_tag, input_header_length, input_content_length);
}

inline void BerView::assign(std::span<const uint8_t> input_ber_data, Tag input_tag, size_t input_header_length,
                            size_t input_content_length) noexcept
{
    m_data           = input_ber_data.data();
    m_header_length  = input_header_length;
    m_content_length = input_content_length;
    m_tag            = input_tag;
}

inline BerViewIterator        BerView::begin() const noexcept { return BerViewIterator{content()}; }
inline BerViewIterator        BerView::end() const noexcept { return BerViewIterator{End::end}; }
inline BerViewIterator        MutableBerView::cbegin() const noexcept { return BerView::begin(); }
inline BerViewIterator        MutableBerView::cend() const noexcept { return BerView::end(); }
inline MutableBerViewIterator MutableBerView::begin() noexcept { return MutableBerViewIterator{content()}; }
inline MutableBerViewIterator MutableBerView::end() noexcept { return MutableBerViewIterator{End::end}; }

inline EncodeResult BerView::encode(std::span<uint8_t> buffer) const noexcept
{
    if (ber_length() > buffer.size())
    {
        return EncodeResult{false, 0};
    }

    std::memcpy(buffer.data(), ber_data(), ber_length());
    return EncodeResult{true, ber_length()};
}

std::ostream& operator<<(std::ostream& os, BerView view) noexcept;

} // namespace fast_ber
