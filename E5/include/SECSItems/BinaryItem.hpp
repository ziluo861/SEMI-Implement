#pragma once
#include <SECS/SECSItem.hpp>
#include <bit>
#include <cstdio>
#include <cstring>

class BinaryItem : public SECSItem<BinaryItem> {
private:
    using type = std::uint8_t;
    std::vector<type> values;
    static constexpr std::uint8_t LogOfElemBytesCount = std::bit_width(sizeof(type)) - 1;
    static constexpr std::uint8_t ElemBytesCount = sizeof(type);
    static constexpr std::uint8_t SizeFilter = sizeof(type) - 1;
public:
    FormatCode GetFormat() const noexcept {
        return FormatCode::BinaryFormatCode;
    }
    std::size_t Size() noexcept {
        return values.size() << LogOfElemBytesCount;
    }
    bool ParseContent(std::string_view _context) {
        auto subStrings = StringUtils::SplitAndRemoveEmpty(_context);
        values.clear();
        values.reserve(subStrings.size());
        for (const auto& sub: subStrings) {
            type _value;
            if (!StringUtils::ParseValue<type>(sub, _value)) {
                return false;
            }
            values.emplace_back(_value);
        }
        return true;
    }

    std::string DeparseContent([[maybe_unused]]int level) const {
        if (values.empty()) {
            return {};
        }
        std::string builder;
        builder.reserve(values.size() * 5);
        for (const auto &_value : values) {
            builder += ' ';
            builder += StringUtils::DeparseValue(_value);
        }
        return builder;
    }
    bool Serialize(std::vector<std::uint8_t>& bytes) const {
        try {
            std::size_t oldSize = bytes.size();
            bytes.reserve(oldSize + values.size() * sizeof(type));
            for (const auto &value : values) {
                bytes.emplace_back(value);
            }
        } catch (...) {
            return false;
        }
        return true;
    }
    bool Deserialize(std::span<std::uint8_t>& bytes, int length) {
        values.clear();
        if (length < 0 || length > static_cast<int>(bytes.size()) || (length & SizeFilter) != 0) {
            return false;
        }
        values.reserve(length / ElemBytesCount);
        for (int i = 0; i < length; i += ElemBytesCount) {
            try {
                auto _bytes = bytes.subspan(i, ElemBytesCount)[0];
                values.emplace_back(_bytes);
            } catch (...) {
                return false;
            }
        }
        bytes = bytes.subspan(length);
        return true;
    }
};