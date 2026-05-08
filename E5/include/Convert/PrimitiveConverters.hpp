#pragma once
#include "SECSBase.hpp"
#include "SECSConverter.hpp"
#include <vector>
#include "ListItem.hpp"
template <typename T>
struct SECSConverter<std::vector<T>>
{
    static bool to(SECSItemBase& item, std::vector<T>& value)
    {
        auto list = dynamic_cast<ListItem*>(&item);
        if (!list)
            return false;

        std::vector<T> result;
        result.reserve(list->Size());

        for (const auto& child : list->Values())
        {
            T temp{};
            if (!SECSItemBase::ConvertTo(*child, temp))
                return false;

            result.push_back(std::move(temp));
        }

        value = std::move(result);
        return true;
    }

    static std::unique_ptr<SECSItemBase> from(const std::vector<T>& values)
    {
        std::vector<std::unique_ptr<SECSItemBase>> subItems;
        subItems.reserve(values.size());

        for (const auto& value : values)
        {
            auto item = SECSItemBase::ConvertFrom(value);
            if (!item)
                return nullptr;

            subItems.push_back(std::move(item));
        }

        return std::make_unique<ListItem>(std::move(subItems));
    }
};