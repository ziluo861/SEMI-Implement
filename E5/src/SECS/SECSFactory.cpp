#include "FormatCode.hpp"
#include "ASCIIItem.hpp"
#include "BinaryItem.hpp"
#include "BooleanItem.hpp"
#include "DoubleItem.hpp"
#include "FloatItem.hpp"
#include "Int16Item.hpp"
#include "Int32Item.hpp"
#include "Int64Item.hpp"
#include "Int8Item.hpp"
#include "ListItem.hpp"
#include "UInt16Item.hpp"
#include "UInt32Item.hpp"
#include "UInt64Item.hpp"
#include "UInt8Item.hpp"
#include <SECSFactory.hpp>
#include <memory>

std::shared_ptr<SECSItemBase> SECSFactory::createItem(FormatCode _code) {
  switch (_code) {
  case FormatCode::ASCIIFormatCode:
    return std::make_shared<ASCIIItem>();
  case FormatCode::ListFormatCode:
    return std::make_shared<ListItem>();
  case FormatCode::BinaryFormatCode:
    return std::make_shared<BinaryItem>();
  case FormatCode::BooleanFormatCode:
    return std::make_shared<BooleanItem>();
  case FormatCode::Int64FormatCode:
    return std::make_shared<Int64Item>();
  case FormatCode::Int8FormatCode:
    return std::make_shared<Int8Item>();
  case FormatCode::Int16FormatCode:
    return std::make_shared<Int16Item>();
  case FormatCode::Int32FormatCode:
    return std::make_shared<Int32Item>();
  case FormatCode::DoubleFormatCode:
    return std::make_shared<DoubleItem>();
  case FormatCode::FloatFormatCode:
    return std::make_shared<FloatItem>();
  case FormatCode::UInt64FormatCode:
    return std::make_shared<UInt64Item>();
  case FormatCode::UInt8FormatCode:
    return std::make_shared<UInt8Item>();
  case FormatCode::UInt32FormatCode:
    return std::make_shared<UInt32Item>();
  case FormatCode::UInt16FormatCode:
    return std::make_shared<UInt16Item>();
  case FormatCode::None:
    return nullptr;
  default:
    return nullptr;
  }
}
