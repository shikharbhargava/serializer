#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <opencv2/opencv.hpp>
#include "Serialization.hpp"

using json = nlohmann::json;

using Serialization::Serializer;
using Serialization::Deserializer;

int i = 1;
std::string str = "ABC123";
std::vector<std::string> strVector = { "A", "B", "C", "D" };
std::list<std::string> strList = { "A", "B", "C", "D" };
std::set<std::string> strSet = { "A", "B", "C", "D" };
std::pair<std::string, int> pair = { "A", 1 };
std::tuple<std::string, int, double> tuple = { "PI", 3, 3.14 };

TEST(Seralization, integer_size)
{
    Serializer serializer;
    serializer.write(i);
    EXPECT_EQ(serializer.dataLength(), sizeof(i));
}

TEST(Seralization, integer_value)
{
    Serializer serializer;
    serializer.write(i);
    int j;
    memcpy(&j, serializer.data(), serializer.dataLength());
    EXPECT_EQ(i, j);
}

TEST(Deseralization, integer_value)
{
    Serializer serializer;
    serializer.write(i);
    int j;
    Deserializer deserializer(serializer.data(), serializer.dataLength());
    deserializer.read(j);
    EXPECT_EQ(i, j);
}

TEST(Seralization, string_size)
{
    Serializer serializer;
    serializer.write(str);
    EXPECT_EQ(serializer.dataLength(), 14);
}

TEST(Seralization, string_value)
{
    Serializer serializer;
    serializer.write(str);
    const auto data = serializer.data();
    std::string strData((const char*)(data + sizeof(uint64_t)), str.length());
    EXPECT_EQ(str, strData);
}

TEST(Deseralization, string_value)
{
    Serializer serializer;
    serializer.write(str);
    std::string strData;
    Deserializer deserializer(serializer.data(), serializer.dataLength());
    deserializer.read(strData);
    EXPECT_EQ(str, strData);
}

TEST(Deseralization, vector_value)
{
    Serializer serializer;
    serializer.write(strVector);
    std::vector<std::string> strVectorData;
    Deserializer deserializer(serializer.data(), serializer.dataLength());
    deserializer.read(strVectorData);
    EXPECT_EQ(strVector, strVectorData);
}

TEST(Deseralization, list_value)
{
    Serializer serializer;
    serializer.write(strList);
    std::list<std::string> strListData;
    Deserializer deserializer(serializer.data(), serializer.dataLength());
    deserializer.read(strListData);
    EXPECT_EQ(strList, strListData);
}

TEST(Deseralization, set_value)
{
    Serializer serializer;
    serializer.write(strSet);
    std::set<std::string> strSetData;
    Deserializer deserializer(serializer.data(), serializer.dataLength());
    deserializer.read(strSetData);
    EXPECT_EQ(strSet, strSetData);
}

TEST(Deseralization, pair_value)
{
    Serializer serializer;
    serializer.write(pair);
    std::pair<std::string, int> pairData;
    Deserializer deserializer(serializer.data(), serializer.dataLength());
    deserializer.read(pairData);
    EXPECT_EQ(pair, pairData);
}

TEST(Deseralization, tuple_value)
{
    Serializer serializer;
    serializer.write(tuple);
    std::tuple<std::string, int, double> tupleData;
    Deserializer deserializer(serializer.data(), serializer.dataLength());
    deserializer.read(tupleData);
    EXPECT_EQ(tuple, tupleData);
}
