#pragma once

#include "fast_ber/util/DecodeHelpers.hpp"
#include "fast_ber/util/EncodeHelpers.hpp"

#include <string_view>
#include <optional>

#include <type_traits>

namespace fast_ber
{

template <typename T, typename DefaultValue>
struct Default
{
    static_assert(std::is_constructible<T, decltype(DefaultValue::get_value())>::value, "Must be convertible");

    Default() noexcept = default;
    Default(const Default& rhs);
    Default(Default&& rhs) noexcept;
    Default(BerView view) { decode(view); }

    Default(const T& val) : m_item((m_default == val) ? std::optional<T>() : std::optional<T>(val)) {}
    template <typename T2>
    Default(const T2& val) : m_item((DefaultValue::get_value() == val) ? std::optional<T>() : std::optional<T>(val))
    {
    }

    Default(const char* val) : Default(std::string_view(val)) // Avoid pointer comparisons for const char*
    {
    }

    ~Default() noexcept = default;

    Default& operator=(const Default&);
    Default& operator=(Default&&) noexcept;
    Default& operator=(const T& val);
    template <typename T2>
    Default& operator=(const T2& val);
    Default& operator=(const char* val);

    const T&              get() const noexcept { return is_default() ? m_default : *m_item; }
    decltype(T{}.value()) value() const noexcept(T{}.value()) { return get()->value(); }
    bool                  is_default() const noexcept { return !m_item; }
    void                  set_to_default() noexcept { m_item = std::nullopt; }

    size_t       encoded_length() const noexcept;
    EncodeResult encode(std::span<uint8_t> buffer) const noexcept;
    DecodeResult decode(BerView input) noexcept;

  private:
    std::optional<T> m_item;
    const T           m_default = T(DefaultValue::get_value());
};

template <typename T, typename DefaultValue>
struct IdentifierType<Default<T, DefaultValue>>
{
    using type = Identifier<T>;
};

template <typename T, typename DefaultValue>
Default<T, DefaultValue>::Default(const Default<T, DefaultValue>& rhs) : m_item(rhs.m_item)
{
}

template <typename T, typename DefaultValue>
Default<T, DefaultValue>::Default(Default<T, DefaultValue>&& rhs) noexcept : m_item(std::move(rhs.m_item))
{
}

template <typename T, typename DefaultValue>
Default<T, DefaultValue>& Default<T, DefaultValue>::operator=(const Default<T, DefaultValue>& rhs)
{
    m_item = rhs.m_item;
    return *this;
}

template <typename T, typename DefaultValue>
Default<T, DefaultValue>& Default<T, DefaultValue>::operator=(Default<T, DefaultValue>&& rhs) noexcept
{
    m_item = std::move(rhs.m_item);
    return *this;
}

template <typename T, typename DefaultValue>
template <typename T2>
Default<T, DefaultValue>& Default<T, DefaultValue>::operator=(const T2& val)
{
    if (DefaultValue::get_value() == val)
    {
        m_item = std::nullopt;
    }
    else
    {
        m_item = val;
    }
    return *this;
}

template <typename T, typename DefaultValue>
Default<T, DefaultValue>& Default<T, DefaultValue>::operator=(const T& val)
{
    if (m_default == val)
    {
        m_item = std::nullopt;
    }
    else
    {
        m_item = val;
    }
    return *this;
}

// Avoid pointer comparisons for const char*
template <typename T, typename DefaultValue>
Default<T, DefaultValue>& Default<T, DefaultValue>::operator=(const char* val)
{
    *this = std::string_view(val);
    return *this;
}

template <typename T, typename DefaultValue>
size_t Default<T, DefaultValue>::encoded_length() const noexcept
{
    if (is_default())
    {
        return 0;
    }
    else
    {
        return m_item->encoded_length();
    }
}

template <typename T, typename DefaultValue>
EncodeResult Default<T, DefaultValue>::encode(std::span<uint8_t> buffer) const noexcept
{
    if (is_default())
    {
        return {true, 0};
    }
    else
    {
        return m_item->encode(buffer);
    }
}

template <typename T, typename DefaultValue>
DecodeResult Default<T, DefaultValue>::decode(BerView input) noexcept
{
    if (input.is_valid() && Identifier<T>::check_id_match(input.class_(), input.tag()))
    {
        m_item = T();
        return m_item->decode(input);
    }
    else if (!input.is_valid())
    {
        this->set_to_default();
        return DecodeResult{true};
    }
    else
    {
        return DecodeResult{false};
    }
}

template <typename T, typename DefaultValue>
DecodeResult decode(BerViewIterator& input, Default<T, DefaultValue>& output) noexcept
{
    if (input->is_valid() && Identifier<T>::check_id_match(input->class_(), input->tag()))
    {
        return output.decode(*input);
    }
    else
    {
        output.set_to_default();
        return DecodeResult{true};
    }
}

template <typename T, typename DefaultValue>
std::ostream& operator<<(std::ostream& os, const Default<T, DefaultValue>& default_type)
{
    return os << default_type.get();
}

template <typename T, typename T2, typename DefaultValue>
bool operator==(const Default<T, DefaultValue>& lhs, const T2& rhs)
{
    return lhs.get() == rhs;
}

template <typename T, typename T2, typename DefaultValue>
bool operator==(const T& lhs, const Default<T2, DefaultValue>& rhs)
{
    return lhs == rhs.get();
}

template <typename T, typename DefaultValue, typename T2, typename DefaultValue2>
bool operator==(const Default<T, DefaultValue>& lhs, const Default<T2, DefaultValue2>& rhs)
{
    return lhs.get() == rhs.get();
}

template <typename T, typename DefaultValue>
bool operator==(const Default<T, DefaultValue>& lhs, const char* rhs)
{
    return lhs.get() == std::string_view(rhs);
}

template <typename T, typename DefaultValue>
bool operator==(const char* lhs, const Default<T, DefaultValue>& rhs)
{
    return std::string_view(lhs) == rhs.get();
}

template <typename T, typename T2, typename DefaultValue>
bool operator!=(const Default<T, DefaultValue>& lhs, const T2& rhs)
{
    return !(lhs == rhs);
}

template <typename T, typename T2, typename DefaultValue>
bool operator!=(const T& lhs, const Default<T2, DefaultValue>& rhs)
{
    return !(lhs == rhs);
}

template <typename T, typename DefaultValue, typename T2, typename DefaultValue2>
bool operator!=(const Default<T, DefaultValue>& lhs, const Default<T2, DefaultValue2>& rhs)
{
    return !(lhs == rhs);
}

template <typename T, typename DefaultValue>
bool operator!=(const Default<T, DefaultValue>& lhs, const char* rhs)
{
    return !(lhs == rhs);
}

template <typename T, typename DefaultValue>
bool operator!=(const char* lhs, const Default<T, DefaultValue>& rhs)
{
    return !(lhs == rhs);
}

} // namespace fast_ber
