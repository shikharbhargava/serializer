#include "Serialization.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <set>
#include <utility>
#include <chrono>

using std::chrono::time_point;
using std::chrono::high_resolution_clock;
using Table = std::vector<std::pair<std::string, int>>;
struct Data
{
    int m_id = 0;
    double m_value = 0.0;
    std::string m_name;
    Table m_table;

    Data() = default;

    Data(int id, double value, const std::string name, const std::vector<std::pair<std::string, int>>& table)
        : m_id(id), m_value(value), m_name(name), m_table(table) {
    }

    Data(int id, double value, const std::string name, std::vector<std::pair<std::string, int>>&& table)
        : m_id(id), m_value(value), m_name(name), m_table(std::move(table)) {
    }

    bool operator==(const Data& other) const
    {
        return (m_id == other.m_id) && (m_value == other.m_value) && (m_name == other.m_name) && (m_table == other.m_table);
    }
};

namespace Serialization
{
    template<>
    inline void Serializer::write<time_point<high_resolution_clock>>(const time_point<high_resolution_clock>& t)
    {
        write(t.time_since_epoch());
    }
    template<>
    inline void Deserializer::read<time_point<high_resolution_clock>>(time_point<high_resolution_clock>& t)
    {
        long long epoch_time;
        read(epoch_time);
        std::chrono::nanoseconds epoch_time_ns(epoch_time);
        t = time_point<high_resolution_clock>(epoch_time_ns);
    }

    template<>
    inline void Serializer::write<Data>(const Data& d)
    {
        write(d.m_id);
        write(d.m_value);
        write(d.m_name);
        write(d.m_table);
    }
    template<>
    inline void Deserializer::read<Data>(Data& d)
    {
        read(d.m_id);
        read(d.m_value);
        read(d.m_name);
        read(d.m_table);
    }
}

using Serialization::Serializer;
using Serialization::Deserializer;
using Strings = std::vector<std::string>;
using DataMap = std::unordered_map<std::string, Data>;

Strings listOfStr;
DataMap dataMap;

int main(int argc, const char** argv)
{
    // Generating data
    auto now = high_resolution_clock::now();
    for (int i = 1; i <= 20; i++)
    {
        std::string id = std::to_string(i);
        listOfStr.push_back(id);
        Table t;
        for (int j = 0; j < i; j++)
            t.push_back({ id, j });
        dataMap.insert({ id, {i, i * 0.1, "name_" + id, std::move(t)} });
    }

    // Serializing data
    Serializer serializer;
    serializer.write(now);
    serializer.write(listOfStr);
    serializer.write(dataMap);
    std::cout << "Total Serialized data size: " << serializer.dataLength() << std::endl;

    // Deserializing data
    time_point<high_resolution_clock> nowDe;
    Strings listOfStrDe;
    DataMap dataMapDe;
    Deserializer deserializer(serializer.data(), serializer.dataLength());
    deserializer.read(nowDe);
    deserializer.read(listOfStrDe);
    deserializer.read(dataMapDe);

    // Cpompairing the deserialized data to original data
    bool isNowSame = now == nowDe;
    std::cout << "isNowSame: " << std::boolalpha << isNowSame << std::endl;
    bool isListOfStrSame = listOfStr == listOfStrDe;
    std::cout << "isListOfStrSame: " << std::boolalpha << isListOfStrSame << std::endl;
    bool isDataMapSame = dataMap == dataMapDe;
    std::cout << "isDataMapSame: " << std::boolalpha << isDataMapSame << std::endl;

    return 0;
}