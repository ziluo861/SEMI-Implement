#pragma once
#include "Converter.hpp"
#include "Int16Item.hpp"
#include "Int32Item.hpp"
#include "Int64Item.hpp"
#include "Int8Item.hpp"
#include "UInt16Item.hpp"
#include "UInt32Item.hpp"
#include "UInt64Item.hpp"
#include "UInt8Item.hpp"
#include "ASCIIItem.hpp"
#include "DoubleItem.hpp"
#include "FloatItem.hpp"
#include "BooleanItem.hpp"
#include <string>

template <>
struct SECSConverter<std::int8_t>
    : ValueItemConverter<Int8Item, std::int8_t>
{
};

template <>
struct SECSConverter<std::int16_t>
    : ValueItemConverter<Int16Item, std::int16_t>
{
};

template <>
struct SECSConverter<std::int32_t>
    : ValueItemConverter<Int32Item, std::int32_t>
{
};

template <>
struct SECSConverter<std::int64_t>
    : ValueItemConverter<Int64Item, std::int64_t>
{
};

template <>
struct SECSConverter<std::uint8_t>
    : ValueItemConverter<UInt8Item, std::uint8_t>
{
};

template <>
struct SECSConverter<std::uint16_t>
    : ValueItemConverter<UInt16Item, std::uint16_t>
{
};

template <>
struct SECSConverter<std::uint32_t>
    : ValueItemConverter<UInt32Item, std::uint32_t>
{
};

template <>
struct SECSConverter<std::uint64_t>
    : ValueItemConverter<UInt64Item, std::uint64_t>
{
};

template <>
struct SECSConverter<std::string>
    : ValueItemConverter<ASCIIItem, std::string>
{
};

template <>
struct SECSConverter<float>
    : ValueItemConverter<FloatItem, float>
{
};

template <>
struct SECSConverter<double>
    : ValueItemConverter<DoubleItem, double>
{
};

template <>
struct SECSConverter<bool>
    : ValueItemConverter<BooleanItem, bool>
{
};