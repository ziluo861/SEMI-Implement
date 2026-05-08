#pragma once
#include "SECSBase.hpp"
#include "SECSConverter.hpp"
#include <memory>
#include <optional>
#include "ListItem.hpp"

template <typename T>
struct SECSConverter<std::optional<T>>
{
    static bool to(SECSItemBase& item, std::optional<T>& value)
    {
        T temp{};

        if (!ConvertTo(item, temp))
        {
            value = std::nullopt;
            return false;
        }

        value = std::move(temp);
        return true;
    }

    static std::unique_ptr<SECSItemBase> from(const std::optional<T>& value)
    {
        if (!value.has_value())
        {
            return std::make_unique<ListItem>();
        }

        return ConvertFrom(*value);
    }
};