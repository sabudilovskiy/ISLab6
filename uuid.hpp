
#include <sstream>
#include <random>
#include <iomanip>
#include <optional>
#pragma once

// Генератор UUID v4

uint32_t GetSeed(std::optional<uint32_t> seed){
    if (seed){
        return *seed;
    }
    return std::random_device{}();
}

struct GeneratorUUID{
    GeneratorUUID(std::optional<uint64_t> seed = std::nullopt) : gen(GetSeed(seed)){

    }
    std::string Next(){
        uint64_t data1 = dis(gen);
        uint64_t data2 = dis(gen);

        // Версия UUID v4 требует, чтобы 13-й символ был "4"
        // и чтобы два старших бита 17-го символа были "10", что означает вариант 1
        data1 = (data1 & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
        data2 = (data2 & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;

        std::stringstream ss;
        ss << std::hex << std::setfill('0')
           << std::setw(8) << (data1 >> 32)
           << "-"
           << std::setw(4) << (data1 >> 16 & 0xFFFF)
           << "-"
           << std::setw(4) << (data1 & 0xFFFF)
           << "-"
           << std::setw(4) << (data2 >> 48)
           << "-"
           << std::setw(12) << (data2 & 0xFFFFFFFFFFFF);

        return ss.str();
    }
private:
    std::mt19937 gen;
    std::uniform_int_distribution<uint64_t> dis;
};