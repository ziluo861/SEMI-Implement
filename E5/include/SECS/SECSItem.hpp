#pragma once
#include <string_view>
#include <SECS/SECSBase.hpp>
#include <Utils/StringUtils.hpp>
#include <sstream>
#include <SECS/SECSFactory.hpp>

template<typename T>
concept _SECSConcept = std::is_class_v<T>;

template<typename Derived>
requires _SECSConcept<Derived>
class SECSItem : public SECSItemBase {
protected:
    SECSItem() = default;
    static constexpr char RangeStartMark = '<';
    static constexpr char RangeEndMark = '>';
    static constexpr std::string_view LevelPrefixElem = "  ";
    static constexpr std::size_t _size = LevelPrefixElem.size();
    static constexpr int maxLevel = 150;
    // static std::string GetPrefix(int level) {
    //     std::ostringstream builder;
    //     for (int i = 0; i < level; i++) {
    //         builder << LevelPrefixElem;
    //     }
    //     return builder.str();
    // }
    static std::string_view GetPrefix(int level) {
        static const std::string prefixes = []() {
            std::string result;
            result.reserve(maxLevel * _size);
            for (int i = 0; i < maxLevel; i++) {
                result += LevelPrefixElem;
            }
            return result;
        }();
    return std::string_view(prefixes.data(), level * _size);
}
public:
    FormatCode TryGetFormat() override {
        return static_cast<Derived*>(this)->GetFormat();
    }
    bool TryParseContent(std::string_view _content) override {
        return static_cast<Derived *>(this)->ParseContent(_content);
    }

    std::size_t Size() noexcept override {
        return static_cast<Derived *>(this)->Size();
    }

    std::string TryDeparseContent(int level = 0) override {
        std::ostringstream builder;
        auto _derived = static_cast<Derived *>(this);
        builder << GetPrefix(level) << RangeStartMark 
        << CodeNameExtension::GetCodeName(_derived->GetFormat()) 
        << _derived->DeparseContent(level) << RangeEndMark;
        return builder.str();
    }
    std::optional<std::vector<std::uint8_t>> TrySerialize() override {
        std::vector<std::uint8_t> builder;
        if (TrySerialize(builder)) {
            return builder;
        }
        return std::nullopt;
    }
    bool TrySerialize(std::vector<std::uint8_t>& bytes) override {
        auto _derived = static_cast<Derived *>(this);
        std::size_t length = _derived->Size();
        if (length == std::numeric_limits<std::size_t>::max()) {
            return false;
        }
        int ByteLength = 0;
        if (length < 0x100) {
            ByteLength = 1;
        } else if (length < 0x10000) {
            ByteLength = 2;
        } else if (length < 0x1000000) {
            ByteLength = 3;
        }
        auto _formatCode = static_cast<int>(_derived->GetFormat());
        bytes.reserve(bytes.size() + ByteLength + 1);
        bytes.emplace_back(static_cast<std::uint8_t>(_formatCode | ByteLength));
        for (int i = ByteLength - 1; i >= 0; i--) {
            std::uint8_t tmp = (length >> (i << 3));
            bytes.emplace_back(static_cast<std::uint8_t>(tmp & 0xFF));
        }
        return _derived->Serialize(bytes);
    }
    bool TryDeserialize(std::span<std::uint8_t>& bytes, int length) override {
        return static_cast<Derived *>(this)->Deserialize(bytes, length);
    }
    virtual ~SECSItem() = default;
};
