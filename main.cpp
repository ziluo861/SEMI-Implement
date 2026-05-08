#include "SECSBase.hpp"
#include "SECSConverter.hpp"
#include "SECSParser.hpp"
#include "ConvertHelper.hpp"
#include <cstdint>
#include <fstream>
#include <cerrno> 
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

int main(int , char **) {

std::ifstream ifs("/Users/luojian/1908/Semi/semiTest.txt", std::ios::binary | std::ios::ate);
    if (!ifs) {
        std::cerr << "open failed, errno=" << errno
                  << " (" << std::strerror(errno) << ")\n";
        return 1;
    }
    std::streamsize size = ifs.tellg();
    std::vector<char> buffer;
    buffer.resize(static_cast<size_t>(size));
    ifs.seekg(0);
    if (!ifs.read(buffer.data(), size)) throw std::runtime_error("read failed");

    std::string dataRaw {buffer.data()};
    std::cout << dataRaw << std::endl;
    auto item = SECSParser::TryParseContent(dataRaw);
    SECSItemBase *it1, *it2;
    if (!ConvertToValues(*item->get(), it1, it2)) {
        std::cout << "parse error";
        return -1;
    }
    std::uint32_t a, b;
    SECSConverter<std::uint32_t>::to(*it1, a);
    SECSConverter<std::uint32_t>::to(*it2, b);
    std::cout << a << " " << b << std::endl;
    return 0;
}