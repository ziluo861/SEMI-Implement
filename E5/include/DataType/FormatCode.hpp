#pragma once
#include <regex>
#include <string>
#include <unordered_map>
#include <array>

enum class FormatCode : int {
  None = -1,
  ListFormatCode = 0x00 << 2,
  BinaryFormatCode = 0x08 << 2,
  BooleanFormatCode = 0x09 << 2,
  ASCIIFormatCode = 0x10 << 2,
  //CharactorFormatCode = 0x12 << 2,
  Int64FormatCode = 0x18 << 2,
  Int8FormatCode = 0x19 << 2,
  Int16FormatCode = 0x1A << 2,
  Int32FormatCode = 0x1C << 2,
  DoubleFormatCode = 0x20 << 2,
  FloatFormatCode = 0x24 << 2,
  UInt64FormatCode = 0x28 << 2,
  UInt8FormatCode = 0x29 << 2,
  UInt16FormatCode = 0x2A << 2,
  UInt32FormatCode = 0x2C << 2,
};

class CodeNameExtension {
private:
  static inline std::unordered_map<std::string, FormatCode> namedCodes;
  static inline std::unordered_map<FormatCode, std::string> codeNames;
  static constexpr std::array<FormatCode, 15> allFormatCodes = {
      FormatCode::None,
      FormatCode::ListFormatCode,
      FormatCode::BinaryFormatCode,
      FormatCode::BooleanFormatCode,
      FormatCode::ASCIIFormatCode,
      //FormatCode::CharactorFormatCode,
      FormatCode::Int64FormatCode,
      FormatCode::Int8FormatCode,
      FormatCode::Int16FormatCode,
      FormatCode::Int32FormatCode,
      FormatCode::DoubleFormatCode,
      FormatCode::FloatFormatCode,
      FormatCode::UInt64FormatCode,
      FormatCode::UInt8FormatCode,
      FormatCode::UInt16FormatCode,
      FormatCode::UInt32FormatCode};

public:
  static std::string getFormatCodeName(FormatCode code);
  static std::unordered_map<FormatCode, std::vector<std::string>> &
  getallFormatMaps();

public:
  static void Initiation();
  static std::string GetCodeName(FormatCode code);
  static FormatCode GetNameCode(const std::string& name);
  static std::regex ParseReg;
};
