#pragma once
#include "SECSConverterImpl.hpp"
#include "PrimitiveConverters.hpp"
#include "SECSBase.hpp"
#include "ListConverter.hpp"

template <typename... Ts>
bool ConvertToValues(SECSItemBase& item, Ts&... outs)
{
    auto list = dynamic_cast<ListItem*>(&item);
    if (!list)
        return false;

    if (list->Size() != sizeof...(Ts))
        return false;

    return ConvertListElements(*list, std::index_sequence_for<Ts...>{}, outs...);
}