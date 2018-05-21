#include "../bin/version.h"

#include <iostream>
#include <string>

#include "mapreduce.h"

int main(int argc, char** argv)
{
    if(argc != 4) {
        std::cout << "Usage " << argv[0] << " src M R" << std::endl;
        return 1;
    }

    std::string src  = argv[1];

    if(src == "-") {
        src = "data.0.tmp";
        std::ofstream out(src);
        std::string line;
        while(std::getline(std::cin, line))
            out << line << std::endl;
    }

    size_t M = std::atol(argv[2]);
    size_t R = std::atol(argv[3]);

    auto map = [](const std::string& line, kvs_t& kvs) {
        for(size_t n = 1; n <= line.length(); ++n)
            kvs.push_back(std::move(std::make_tuple(line.substr(0, n), "")));
    };

    auto reduce = [](const kvs_t& kvs, strings_t& lines) {
        size_t dup_len = 0;
        auto it = kvs.begin();
        while(it != kvs.end()) {
            auto itc = it;
            const std::string& key = std::get<0>(*itc);

            it = std::find_if_not(itc, kvs.end(), [key](const kv_t& kv) {
                return key == std::get<0>(kv);
            });

            if(it - itc > 1 && key.length() > dup_len)
                dup_len = key.length();

        }
        lines.push_back(std::to_string(dup_len + 1));
    };

    mapreduce(src, "-", M, R, map, reduce);

    return 0;

}
