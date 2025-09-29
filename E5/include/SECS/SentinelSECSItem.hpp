#pragma once

#include "DataType/FormatCode.hpp"
#include "SECS/SECSBase.hpp"
#include <optional>

class SentinelSECSItem final : public SECSItemBase {
    FormatCode TryGetFormat() override {
        return FormatCode::None;
    }
    bool TryParseContent(std::string_view) override {
        return false;
    }
    std::string TryDeparseContent([[maybe_unused]] int level = 0) override {
        return {};
    }
    std::optional<std::vector<std::uint8_t>> TrySerialize() override {
        return std::nullopt;
    }
    bool TrySerialize(std::vector<std::uint8_t>&) override {
        return false;
    }
    bool TryDeserialize(std::span<std::uint8_t>&, int) override {
        return false;;
    }
    
};