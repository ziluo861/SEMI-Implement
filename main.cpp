#include "ListItem.hpp"
#include "SECSBase.hpp"
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
    std::uint32_t a{}, b{};
    const SECSItemBase* subListItem{};
    bool result = ConvertToValues(*item->get(), a, b, subListItem);
    if (!result) {
        std::cout << "ConvertToValues failed\n";
    }
    std::cout << "a = " << a << ", b = " << b << std::endl;

    //result = ConvertToValues(subListItem->get())
    return 0;
}