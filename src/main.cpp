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

    // stage 1: deduplication
    auto map1 = [](const std::string& line, kvs_t& kvs) {
        kvs.push_back(std::move(std::make_tuple(line, "")));
    };

    auto reduce1 = [](const kvs_t& kvs, strings_t& lines) {
        lines.push_back(std::get<0>(kvs.front()));
    };

    mapreduce(src, "data.1.tmp", M, R, map1, reduce1);

    // stage 2 : unique substring + full string
    auto map2 = [](const std::string& line, kvs_t& kvs) {
        for(size_t n = 1; n <= line.length(); ++n)
            kvs.push_back(std::move(std::make_tuple(line.substr(0, n), line)));
    };

    auto reduce2 = [](const kvs_t& kvs, strings_t& lines) {
        if(kvs.size() == 1)
            lines.push_back(std::get<1>(kvs.front()) + "\t" + std::get<0>(kvs.front()));
        else
            for(auto& kv : kvs)
                if(std::get<1>(kv) == std::get<0>(kv))
                    lines.push_back(std::get<1>(kv) + "\t" + std::get<0>(kv));
    };

    mapreduce("data.1.tmp", "data.2.tmp", M, R, map2, reduce2);

    // stage 3 : shortest substring
    auto map3 = [](const std::string& line, kvs_t& kvs) {
        size_t n = line.find('\t');
        kvs.push_back(std::make_tuple(line.substr(0, n), line.substr(n+1, line.length() - n - 1)));
    };

    auto reduce3 = [](const kvs_t& kvs, strings_t& lines) {
        auto f = kvs.end();
        for(auto it = kvs.begin(); it != kvs.end(); ++it)
            if(f == kvs.end() || std::get<1>(*f).length() > std::get<1>(*it).length())
                f = it;
        if(f != kvs.end())
            lines.push_back(std::get<1>(*f) + "\t" + std::get<0>(*f));
    };

    mapreduce("data.2.tmp", "-", M, R, map3, reduce3);

    return 0;

}
