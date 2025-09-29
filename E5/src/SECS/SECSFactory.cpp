#include "DataType/FormatCode.hpp"
#include "SECSItems/ASCIIItem.hpp"
#include "SECSItems/BinaryItem.hpp"
#include "SECSItems/BooleanItem.hpp"
#include "SECSItems/DoubleItem.hpp"
#include "SECSItems/FloatItem.hpp"
#include "SECSItems/Int16Item.hpp"
#include "SECSItems/Int32Item.hpp"
#include "SECSItems/Int64Item.hpp"
#include "SECSItems/Int8Item.hpp"
#include "SECSItems/ListItem.hpp"
#include "SECSItems/UInt16Item.hpp"
#include "SECSItems/UInt32Item.hpp"
#include "SECSItems/UInt64Item.hpp"
#include "SECSItems/UInt8Item.hpp"
#include <SECS/SECSFactory.hpp>

std::unique_ptr<SECSItemBase> SECSFactory::createItem(FormatCode _code) {
  switch (_code) {
  case FormatCode::ASCIIFormatCode:
    return std::make_unique<ASCIIItem>();
  case FormatCode::ListFormatCode:
    return std::make_unique<ListItem>();
  case FormatCode::BinaryFormatCode:
    return std::make_unique<BinaryItem>();
  case FormatCode::BooleanFormatCode:
    return std::make_unique<BooleanItem>();
  case FormatCode::Int64FormatCode:
    return std::make_unique<Int64Item>();
  case FormatCode::Int8FormatCode:
    return std::make_unique<Int8Item>();
  case FormatCode::Int16FormatCode:
    return std::make_unique<Int16Item>();
  case FormatCode::Int32FormatCode:
    return std::make_unique<Int32Item>();
  case FormatCode::DoubleFormatCode:
    return std::make_unique<DoubleItem>();
  case FormatCode::FloatFormatCode:
    return std::make_unique<FloatItem>();
  case FormatCode::UInt64FormatCode:
    return std::make_unique<UInt64Item>();
  case FormatCode::UInt8FormatCode:
    return std::make_unique<UInt8Item>();
  case FormatCode::UInt32FormatCode:
    return std::make_unique<UInt32Item>();
  case FormatCode::UInt16FormatCode:
    return std::make_unique<UInt16Item>();
  case FormatCode::None:
    return nullptr;
  default:
    return nullptr;
  }
}
