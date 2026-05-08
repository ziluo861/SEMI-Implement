#pragma once
#include <FormatCode.hpp>
#include <SECSBase.hpp>
#include <memory>
class SECSFactory {
public:
  static std::shared_ptr<SECSItemBase> createItem(FormatCode _code);
};
