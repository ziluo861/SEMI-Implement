#pragma once

#include "SECSBase.hpp"

template <typename ItemType, typename ValueType>
struct ValueItemConverter
{
    static bool to(const SECSItemBase& item, ValueType& value)
    {
        auto target = dynamic_cast<const ItemType*>(&item);
        if (!target)
            return false;

        value = target->Value();
        return true;
    }

    static std::unique_ptr<SECSItemBase> from(ValueType value)
    {
        return std::make_unique<ItemType>(value);
    }
};