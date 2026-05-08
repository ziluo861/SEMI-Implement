#pragma once
#include "SECSConverterImpl.hpp"
#include "SECSBase.hpp"
#include "ListConverter.hpp"

template <typename... Ts>
bool ConvertToValues(const SECSItemBase& item, Ts&... outs)
{
    auto list = dynamic_cast<const ListItem*>(&item);
    if (!list)
        return false;

    if (list->Count() != sizeof...(Ts))
        return false;

    return ConvertListElements(*list, std::index_sequence_for<Ts...>{}, outs...);
}