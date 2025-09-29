#include "DataType/FormatCode.hpp"

std::regex CodeNameExtension::ParseReg(R"(^([a-zA-Z0-9]|\||\*|\?)+)",
                                       std::regex_constants::icase |
                                           std::regex_constants::optimize);

std::string CodeNameExtension::getFormatCodeName(FormatCode code) {
  switch (code) {
  case FormatCode::None:
    return "None";
  case FormatCode::ListFormatCode:
    return "ListFormatCode";
  case FormatCode::BinaryFormatCode:
    return "BinaryFormatCode";
  case FormatCode::BooleanFormatCode:
    return "BooleanFormatCode";
  case FormatCode::ASCIIFormatCode:
    return "ASCIIFormatCode";
  // case FormatCode::CharactorFormatCode:
  //   return "CharactorFormatCode";
  case FormatCode::Int64FormatCode:
    return "Int64FormatCode";
  case FormatCode::Int8FormatCode:
    return "Int8FormatCode";
  case FormatCode::Int16FormatCode:
    return "Int16FormatCode";
  case FormatCode::Int32FormatCode:
    return "Int32FormatCode";
  case FormatCode::DoubleFormatCode:
    return "DoubleFormatCode";
  case FormatCode::FloatFormatCode:
    return "FloatFormatCode";
  case FormatCode::UInt64FormatCode:
    return "UInt64FormatCode";
  case FormatCode::UInt8FormatCode:
    return "UInt8FormatCode";
  case FormatCode::UInt16FormatCode:
    return "UInt16FormatCode";
  case FormatCode::UInt32FormatCode:
    return "UInt32FormatCode";
  default:
    return "Unknown";
  }
}

std::unordered_map<FormatCode, std::vector<std::string>> &
CodeNameExtension::getallFormatMaps() {
  static std::unordered_map<FormatCode, std::vector<std::string>>
      allFormatMaps = {
          {FormatCode::None, {"Error"}},
          {FormatCode::ListFormatCode, {"L", "List", "list", "LIST"}},
          {FormatCode::BinaryFormatCode,
           {"B", "Byte", "Binary", "byte", "binary"}},
          {FormatCode::BooleanFormatCode,
           {"BOOLEAN", "Bool", "Boolean", "bool", "boolean"}},
          {FormatCode::ASCIIFormatCode, {"A", "ASCII", "ascii", "string"}},
          //{FormatCode::CharactorFormatCode, {"C", "Charactor"}},
          {FormatCode::Int8FormatCode, {"I1", "Int8", "i1", "int8"}},
          {FormatCode::Int16FormatCode, {"I2", "i2", "Int16", "int16"}},
          {FormatCode::Int32FormatCode, {"I4", "i4", "Int32", "int32"}},
          {FormatCode::Int64FormatCode, {"I8", "Int64", "i8", "int64"}},
          {FormatCode::UInt8FormatCode,
           {"U1", "u1", "UInt8", "Uint8", "uint8"}},
          {FormatCode::UInt16FormatCode,
           {"U2", "u2", "UInt16", "Uint16", "uint16"}},
          {FormatCode::UInt32FormatCode,
           {"U4", "u4", "UInt32", "Uint32", "uint32"}},
          {FormatCode::UInt64FormatCode,
           {"U8", "u8", "UInt64", "Uint64", "uint64"}},
          {FormatCode::DoubleFormatCode, {"F8", "D", "Double", "double"}},
          {FormatCode::FloatFormatCode, {"F4", "F", "Float", "float"}},
      };
  return allFormatMaps;
}

__attribute__((constructor)) static void InitFormatCodeExtension() {
  CodeNameExtension::Initiation();
}

void CodeNameExtension::Initiation() {
  const auto &allFormatMaps = getallFormatMaps();
  codeNames.reserve(allFormatCodes.size());
  for (const auto &code : allFormatCodes) {
    auto names = allFormatMaps.find(code);
    if (names == allFormatMaps.end()) {
      throw std::out_of_range("allFormatMaps");
    }
    codeNames[names->first] = names->second.front();
    namedCodes.reserve(names->second.size());
    for (auto &name : names->second) {
      namedCodes[name] = names->first;
    }
  }
  return;
}

std::string CodeNameExtension::GetCodeName(FormatCode code) {
  auto it = codeNames.find(code);
  if (it == codeNames.end()) {
    throw std::out_of_range("Key not find in codeNames");
  }
  return it->second;
}

FormatCode CodeNameExtension::GetNameCode(const std::string &name) {
  if (name.empty()) {
    throw std::invalid_argument("name is empty");
  }
  auto it = namedCodes.find(name);
  if (it != namedCodes.end()) {
    return it->second;
  }
  throw std::out_of_range("Key not find in namedCodes");
}
