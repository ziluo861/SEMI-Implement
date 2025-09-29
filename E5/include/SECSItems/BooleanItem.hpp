#pragma once
#include <SECS/SECSItem.hpp>
#include <bit>

class BooleanItem : public SECSItem<BooleanItem> {
private:
    using type = char;
    std::vector<type> values;
    static constexpr std::uint8_t LogOfElemBytesCount = std::bit_width(sizeof(type)) - 1;
    static constexpr std::uint8_t ElemBytesCount = sizeof(type);
    static constexpr std::uint8_t SizeFilter = sizeof(type) - 1;
    static bool ParseValue(std::string_view context, type &result) noexcept {
        if (context.size() == 4) {
             if (!std::equal(context.begin(), context.end(), "true",
                [](char a, char b) { return std::tolower(a) == b; })) {
                    return false;
                }
             result = 1;
             return true;
        }
        if (context.size() == 5) {
            if (!std::equal(context.begin(), context.end(), "false",
                [](char a, char b) { return std::tolower(a) == b; })) {
                    return false;
                }
            result = 0;
            return true;
        }
        return false;
    }
    static constexpr std::uint8_t trueBytes = 1;
    static constexpr std::uint8_t falseBytes = 0;
public:
    FormatCode GetFormat() const noexcept{
        return FormatCode::BooleanFormatCode;
    }
    std::size_t Size()  noexcept{
        return values.size() << LogOfElemBytesCount;
    }
    bool ParseContent(std::string_view _context) {
        auto subStrings = StringUtils::SplitAndRemoveEmpty(_context);
        values.clear();
        values.reserve(subStrings.size());
        for (const auto& sub: subStrings) {
            type _value;
            if (!ParseValue(sub, _value)) {
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
        builder.reserve(values.size() * 7);
        for (const auto &_value : values) {
            builder += ' ';
            //builder += StringUtils::DeparseValue(static_cast<bool>(_value));
            builder += (_value ? "true" : "false");
        }
        return builder;
    }
    bool Serialize(std::vector<std::uint8_t>& bytes) const {
        try {
            bytes.reserve(bytes.size() + values.size());
            // for (const auto &_it : values) {
            //     bytes.emplace_back(_it ? trueBytes : falseBytes);
            // }
            bytes.insert(bytes.end(), values.begin(), values.end());
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
        // for (int i = 0; i < length; i += ElemBytesCount) {
        //     try {
        //         auto _byte = bytes.subspan(i, ElemBytesCount)[0];
        //         values.emplace_back(_byte == trueBytes);
        //     } catch (...) {
        //         return false;
        //     }
        // }
        values.assign(bytes.begin(), bytes.begin() + length);
        bytes = bytes.subspan(length);
        return true;
    }
};