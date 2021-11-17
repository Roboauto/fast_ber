#pragma once

#include "fast_ber/util/DecodeHelpers.hpp"
#include "fast_ber/util/EncodeHelpers.hpp"
#include "fast_ber/util/FixedIdBerContainer.hpp"

#include <vector>
#include <span>

namespace fast_ber
{

using ObjectIdentifierComponents = std::vector<int64_t>;

size_t       encoded_object_id_length(const ObjectIdentifierComponents& input) noexcept;
EncodeResult encode_object_id(std::span<uint8_t> output, const ObjectIdentifierComponents& input,
                              size_t encoded_length = 0) noexcept;
DecodeResult decode_object_id(std::span<const uint8_t> input, ObjectIdentifierComponents& output) noexcept;
// Return -1 on fail
int64_t get_component_number(std::span<const uint8_t> input, size_t component_number) noexcept;
size_t  get_number_of_components(std::span<const uint8_t> input) noexcept;

template <typename Identifier = ExplicitId<UniversalTag::object_identifier>>
class ObjectIdentifier
{
  public:
    ObjectIdentifier() noexcept                                   = default;
    ObjectIdentifier(const ObjectIdentifier<Identifier>& rhs)     = default;
    ObjectIdentifier(ObjectIdentifier<Identifier>&& rhs) noexcept = default;
    ObjectIdentifier(const ObjectIdentifierComponents& oid) noexcept { assign(oid); }
    ObjectIdentifier(const std::initializer_list<int64_t>& oid) noexcept { assign(ObjectIdentifierComponents(oid)); }
    explicit ObjectIdentifier(BerView view) { decode(view); }
    ~ObjectIdentifier() noexcept = default;

    ObjectIdentifier<Identifier>& operator=(const ObjectIdentifierComponents& rhs) noexcept;
    ObjectIdentifier&             operator=(const ObjectIdentifier&) = default;
    ObjectIdentifier&             operator=(ObjectIdentifier&&) noexcept = default;

    template <typename Identifier2>
    bool operator==(const ObjectIdentifier<Identifier2>& rhs) const noexcept;
    template <typename Identifier2>
    bool operator!=(const ObjectIdentifier<Identifier2>& rhs) const noexcept;
    bool operator==(const ObjectIdentifierComponents& rhs) const noexcept { return this->value() == rhs; }
    bool operator!=(const ObjectIdentifierComponents& rhs) const noexcept { return !(*this == rhs); }

    size_t  number_of_components() const noexcept;
    int64_t component_number(size_t number_of_component_to_extract) const noexcept;

    ObjectIdentifierComponents value() const noexcept;

    bool assign(const ObjectIdentifierComponents& oid) noexcept;

    size_t       encoded_length() const noexcept;
    EncodeResult encode(std::span<uint8_t> buffer) const noexcept;
    DecodeResult decode(BerView buffer) noexcept;

    using AsnId = Identifier;

  private:
    FixedIdBerContainer<Identifier> m_contents;
};

inline size_t encoded_object_id_length(const ObjectIdentifierComponents& input) noexcept
{
    if (input.size() < 2)
    {
        return 0;
    }

    size_t output_size = 1;
    for (size_t i = 2; i < input.size(); i++)
    {
        int64_t component = input[i];
        do
        {
            output_size++;
            component /= 128;
        } while (component > 0);
    }
    return output_size;
}

inline EncodeResult encode_object_id(std::span<uint8_t> output, const ObjectIdentifierComponents& input,
                                     size_t encoded_length) noexcept
{
    if (encoded_length == 0)
    {
        encoded_length = encoded_object_id_length(input);
    }

    if (output.empty() || output.size() < encoded_length || input.size() < 2 || input[0] > 2 || input[1] > 39)
    {
        return EncodeResult{false, 0};
    }

    output[0] = static_cast<uint8_t>(40 * input[0] + input[1]);

    size_t output_write_pos = encoded_length - 1;
    for (size_t i = input.size() - 1; i > 1; i--)
    {
        int64_t component = input[i];
        if (component < 0)
        {
            return EncodeResult{false, 0};
        }

        bool first = true;
        do
        {
            output[output_write_pos] = static_cast<uint8_t>(component % 128);
            if (first)
            {
                first = false;
            }
            else
            {
                output[output_write_pos] |= 0x80;
            }

            output_write_pos--;
            component /= 128;
        } while (component > 0);
    }

    return EncodeResult{true, encoded_length};
}

inline DecodeResult decode_object_id(std::span<const uint8_t> input, std::vector<int64_t>& output) noexcept
{
    if (!output.empty())
    {
        output.clear();
    }

    if (input.empty())
    {
        return DecodeResult{false};
    }

    output.push_back(input[0] / 40);
    output.push_back(input[0] % 40);

    for (size_t i = 1, current_num = 0; i < input.size(); i++)
    {
        current_num += 0x7F & input[i];
        if (0x80 & input[i])
        {
            current_num *= 0x80;
        }
        else
        {
            output.push_back(current_num);
            current_num = 0;
        }
    }

    return DecodeResult{true};
}

inline int64_t get_component_number(std::span<const uint8_t> input, size_t number_of_component_to_extract) noexcept
{
    if (input.empty())
    {
        return -1;
    }

    if (number_of_component_to_extract == 0)
    {
        return input[0] / 40;
    }
    if (number_of_component_to_extract == 1)
    {
        return input[1] % 40;
    }

    for (size_t i = 1, current_num = 0, current_component_number = 2; i < static_cast<size_t>(input.size()); i++)
    {
        current_num += 0x7F & input[i];
        if (0x80 & input[i])
        {
            current_num *= 0x80;
        }
        else
        {
            current_num = 0;
            if (current_component_number == number_of_component_to_extract)
            {
                return current_num;
            }
            current_component_number++;
        }
    }

    return -1;
}

inline size_t get_number_of_components(std::span<const uint8_t> input) noexcept
{
    if (input.empty())
    {
        return 0;
    }

    size_t number_of_components = 2;
    for (size_t i = 1; i < input.size(); i++)
    {
        if (!(0x80 & input[i]))
        {
            number_of_components++;
        }
    }

    return number_of_components;
}

template <typename Identifier>
ObjectIdentifier<Identifier>& ObjectIdentifier<Identifier>::operator=(const ObjectIdentifierComponents& rhs) noexcept
{
    assign(rhs);
    return *this;
}

template <typename Identifier>
template <typename Identifier2>
bool ObjectIdentifier<Identifier>::operator==(const ObjectIdentifier<Identifier2>& rhs) const noexcept
{
    auto lhc = this->m_contents.content();
    auto rhc = rhs.m_contents.content();
    return std::equal(lhc.begin(), lhc.end(), rhc.begin(), rhc.end());
}

template <typename Identifier>
template <typename Identifier2>
bool ObjectIdentifier<Identifier>::operator!=(const ObjectIdentifier<Identifier2>& rhs) const noexcept
{
    return !(*this == rhs);
}

template <typename Identifier>
std::ostream& operator<<(std::ostream& os, const ObjectIdentifier<Identifier>& oid) noexcept
{
    os << "[";
    const auto oid_components = oid.value();
    for (size_t i = 0; i < oid_components.size(); i++)
    {
        os << oid_components[i];
        if (i < oid_components.size() - 1)
        {
            os << ", ";
        }
    }
    os << "]";
    return os;
}

template <typename Identifier>
size_t ObjectIdentifier<Identifier>::number_of_components() const noexcept
{
    return fast_ber::get_number_of_components(m_contents.content());
}

template <typename Identifier>
int64_t ObjectIdentifier<Identifier>::component_number(size_t component_number) const noexcept
{
    return fast_ber::get_component_number(m_contents.content(), component_number);
}

template <typename Identifier>
ObjectIdentifierComponents ObjectIdentifier<Identifier>::value() const noexcept
{
    ObjectIdentifierComponents output;
    decode_object_id(m_contents.content(), output);
    return output;
}

template <typename Identifier>
bool ObjectIdentifier<Identifier>::assign(const ObjectIdentifierComponents& oid) noexcept
{
    const size_t encoded_length = encoded_object_id_length(oid);
    m_contents.resize_content(encoded_length);
    EncodeResult res = encode_object_id(std::span<uint8_t>(m_contents.content()), oid, encoded_length);
    if (res.success)
    {
        assert(res.length == m_contents.content_length());
    }
    return res.success;
}

template <typename Identifier>
size_t ObjectIdentifier<Identifier>::encoded_length() const noexcept
{
    return this->m_contents.ber_length();
}

template <typename Identifier>
EncodeResult ObjectIdentifier<Identifier>::encode(std::span<uint8_t> output) const noexcept
{
    return this->m_contents.encode(output);
}

template <typename Identifier>
DecodeResult ObjectIdentifier<Identifier>::decode(BerView input) noexcept
{
    return this->m_contents.decode(input);
}

} // namespace fast_ber
