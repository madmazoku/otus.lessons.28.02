#pragma once

#include <algorithm>
#include <functional>
#include <string>
#include <fstream>
#include <tuple>
#include <vector>
#include <thread>
#include <mutex>
#include <memory>
#include <chrono>

// Travis do not have it
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

using kv_t = std::tuple<std::string, std::string>;
using kvs_t = std::vector<kv_t>;
using strings_t = std::vector<std::string>;
using reduce_data_t = std::tuple<std::unique_ptr<std::mutex>, kvs_t, std::thread>;
using map_t =  std::function<void(const std::string& in, kvs_t& out)>;
using reduce_t =  std::function<void(const kvs_t& in, strings_t& out)>;

void mapreduce(const std::string& fnin, const std::string& fnout, size_t M, size_t R, const map_t& map, const reduce_t& reduce)
{

    std::ifstream in(fnin, std::ifstream::ate | std::ifstream::binary);
    size_t file_size = in.tellg();

    std::vector<std::thread> mappers;
    std::vector<reduce_data_t> reducers;

    mappers.resize(M);
    reducers.resize(R);
    for(auto& r : reducers)
        std::get<0>(r) = std::move(make_unique<std::mutex>());

    size_t n = M + 1;
    size_t start = 0;
    while(start < file_size) {
        size_t length = (file_size - start) / --n;
        if(length == 0)
            length = 1;

        in.seekg(start + length);
        while(!in.eof()) {
            ++length;
            if(in.get() == '\n')
                break;
        }

        mappers[n-1] = std::thread([&fnin, R, map, &reducers, start, length]() {
            std::ifstream in(fnin, std::ifstream::binary);
            in.seekg(start);

            std::hash<std::string> hash_fn;
            std::vector<kvs_t> rd;
            rd.resize(R);

            kvs_t kvs;
            std::string line;
            while(in.tellg() < start + length && std::getline(in, line)) {
                map(line, kvs);
                for(auto& kv : kvs)
                    rd[hash_fn(std::get<0>(kv)) % R].push_back(kv);
                kvs.clear();
            }
            for(size_t r = 0; r < R; ++r) {
                std::lock_guard<std::mutex> rlock(*(std::get<0>(reducers[r])));
                std::copy(rd[r].begin(), rd[r].end(), std::back_inserter(std::get<1>(reducers[r])));
            }
        });

        start += length ;
    }

    for(size_t m = n-1; m < M; ++m)
        mappers[m].join();

    std::mutex lmutex;
    strings_t lines;

    for(auto& r : reducers) {
        std::get<2>(r) = std::thread([reduce, &r, &lmutex, &lines]() {
            auto b = std::get<1>(r).begin();
            auto e = std::get<1>(r).end();
            std::sort(b, e, [](kv_t a, kv_t b) {
                return std::get<0>(a) < std::get<0>(b);
            });

            strings_t ls;
            reduce(std::get<1>(r), ls);

            std::lock_guard<std::mutex> llock(lmutex);
            std::copy(ls.begin(), ls.end(), std::back_inserter(lines));
        });
    }

    for(auto& r : reducers)
        std::get<2>(r).join();

    if(fnout == "-") {
        for(auto& l : lines)
            std::cout << l << std::endl;
    } else {
        std::ofstream out(fnout);
        for(auto& l : lines)
            out << l << std::endl;
    }
}