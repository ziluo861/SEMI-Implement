#pragma once
#include <DataType/FormatCode.hpp>
#include <SECS/SECSBase.hpp>
#include <memory>
class SECSFactory {
public:
  static std::shared_ptr<SECSItemBase> createItem(FormatCode _code);
};
