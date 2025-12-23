// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <cctype>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <geodesk/geodesk.h>

using namespace geodesk;

template<typename Collection>
void queryAndDisplay(Collection collection, uint64_t id, const char* typeName)
{
    auto start = std::chrono::high_resolution_clock::now();
    auto feature = collection.byId(id);
    auto end = std::chrono::high_resolution_clock::now();
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    if (feature)
    {
        std::cout << typeName << " " << id << " (" << us << " us):" << std::endl;
        for (Tag tag : feature->tags())
        {
            std::cout << "  " << tag.key() << " = " << tag.value() << std::endl;
        }
    }
    else
    {
        std::cout << typeName << " " << id << " not found (" << us << " us)" << std::endl;
    }
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <gol-file> [n|w|r<id>]" << std::endl;
        std::cerr << "  Example: " << argv[0] << " planet.gol w12345" << std::endl;
        return 1;
    }

    Features features(argv[1]);
    std::cout << "Loaded " << argv[1] << std::endl;

    if (argc >= 3)
    {
        const char* arg = argv[2];
        char typeChar = std::tolower(static_cast<unsigned char>(arg[0]));
        uint64_t id = std::strtoull(arg + 1, nullptr, 10);

        try
        {
            switch (typeChar)
            {
                case 'n': queryAndDisplay(features.nodes(), id, "Node"); break;
                case 'w': queryAndDisplay(features.ways(), id, "Way"); break;
                case 'r': queryAndDisplay(features.relations(), id, "Relation"); break;
                default:
                    std::cerr << "Unknown type '" << arg[0] << "'. Use n, w, or r." << std::endl;
                    return 1;
            }
        }
        catch (const QueryException& e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
    }

    return 0;
}
