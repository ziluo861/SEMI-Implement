#pragma once
#include "SECSBase.hpp"
#include "ListItem.hpp"

inline bool ConvertOneListElement(
    const ListItem& list,
    std::size_t index,
    SECSItemBase*& out)
{
    if (index >= list.Count()) {
        out = nullptr;
        return false;
    }
    auto& values = list.Values();
    out = values[index].get();
    return out != nullptr;
}

template <typename T>
bool ConvertOneListElement(
    const ListItem& list,
    std::size_t index,
    T& out)
{
    SECSItemBase* child = nullptr;

    if (!ConvertOneListElement(list, index, child)) {
        out = T{};
        return false;
    }

    return SECSItemBase::ConvertTo(*child, out);
}

template <typename... Ts, std::size_t... Is>
bool ConvertListElements(
    const ListItem& list,
    std::index_sequence<Is...>,
    Ts&... outs)
{
    return (ConvertOneListElement(list, Is, outs) && ...);
}
