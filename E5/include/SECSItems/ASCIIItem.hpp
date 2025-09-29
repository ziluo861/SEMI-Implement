#pragma once
#include <SECS/SECSItem.hpp>
#include <string_view>
#include <string>
#include <Utils/StringUtils.hpp>

class ASCIIItem : public SECSItem<ASCIIItem> {
private:
    std::string value;
public:
    FormatCode GetFormat() const noexcept{
        return FormatCode::ASCIIFormatCode;
    }
    bool ParseContent(std::string_view _context) {
        value = StringUtils::trim(_context);
        return true;
    }
    std::string DeparseContent([[maybe_unused]]int level) const {
        return ' ' + value;
    }
    std::size_t Size() noexcept{
        return value.size();
    }
    bool Serialize(std::vector<std::uint8_t>& bytes) const {
        if (value.empty()) {
            return true;
        }
        bytes.reserve(bytes.size() + value.size());
        bytes.insert(bytes.end(), value.begin(), value.end());
        return true;
    }
    bool Deserialize(std::span<std::uint8_t>& bytes, int length) {
        if (length < 0 || length > static_cast<int>(bytes.size())) {
            return false;
        }
        value.reserve(length);
        value.assign(reinterpret_cast<const char*>(bytes.data()), length);
        bytes = bytes.subspan(length);
        return true;
    }
};