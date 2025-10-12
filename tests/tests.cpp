#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <opencv2/opencv.hpp>

#include "keyValueParser.h"

using json = nlohmann::json;

// KeyValueParser Tests

TEST(KeyValueParserTest, GetValue)
{
  Parsers::KeyValueParser::KeyValueMap rules = {
    {"key1", "value1"},
    {"key2", "42"},
    {"key3", "3.14"},
    {"key4", "true"},
    {"key5", "0"},
    {"key6", "1,2,3,4,5"}
  };
  Parsers::KeyValueParser parser(rules);

  // Test getValue
  EXPECT_EQ(parser.getValue<std::string>("key1"), "value1");
  EXPECT_EQ(parser.getValue<int>("key2"), 42);
  EXPECT_DOUBLE_EQ(parser.getValue<double>("key3"), 3.14);
  EXPECT_EQ(parser.getValue<bool>("key4"), true);
  EXPECT_EQ(parser.getValue<bool>("key5"), false);

  // Test getValueList
  auto list = parser.getValueList<int>("key6", ',');
  std::vector<int> expectedList = {1, 2, 3, 4, 5};
  EXPECT_EQ(list, expectedList);

  // Test unknown key
  EXPECT_THROW(parser.getValue<std::string>("unknown_key"), std::invalid_argument);
}

// XML Tests

TEST(XMLTest, POINTS)
{
  std::string xmlPoints = R"(
    <MARK_POINT>
      <POINTS>3</POINTS>
      <P1> 296,1015 </P1>
      <P2> 916,00 </P2>
      <P3> 1096,32 </P3>
    </MARK_POINT>
  )";
  auto points = Parsers::KeyValueParser::parseValue<Parsers::KeyValueParser::ROI, Parsers::KeyValueParser::XML, std::vector<cv::Point>>(xmlPoints);
  ASSERT_EQ(points.size(), 3);
  EXPECT_EQ(points[0], cv::Point(296, 1015));
  EXPECT_EQ(points[1], cv::Point(916, 0));
  EXPECT_EQ(points[2], cv::Point(1096, 32));
}

TEST(XMLTest, ROI_POINTS) {
  std::string xmlRois = R"(
    <MARKED_ROI id="SLOTS">
      <NUM_ROI>2</NUM_ROI>
      <ROI id="0">
        <ROI_NAME>S1</ROI_NAME>
        <MARK_POINT>
          <POINTS>3</POINTS>
          <P1> 120,80 </P1>
          <P2> 520,80 </P2>
          <P3> 520,380 </P3>
        </MARK_POINT>
      </ROI>
      <ROI id="1">
        <ROI_NAME>SLOT2</ROI_NAME>
        <MARK_POINT>
          <POINTS>4</POINTS>
          <P1> 640,200 </P1>
          <P2> 940,200 </P2>
          <P3> 940,600 </P3>
          <P4> 640,600 </P4>
        </MARK_POINT>
      </ROI>
    </MARKED_ROI>
  )";

  auto [markedPointAttributes, roiPointsList] = Parsers::KeyValueParser::parseValue<Parsers::KeyValueParser::ROI, Parsers::KeyValueParser::XML, Parsers::MarkedROIPoints>(xmlRois);
  ASSERT_EQ(markedPointAttributes.size(), 1);
  ASSERT_TRUE(markedPointAttributes.find("id") != markedPointAttributes.end());
  auto id = markedPointAttributes.at("id");
  EXPECT_EQ(id, "SLOTS");
  ASSERT_EQ(roiPointsList.size(), 2);
  {
    const auto& [roiKeyValues, roiPoints] = roiPointsList[0];
    ASSERT_EQ(roiKeyValues.size(), 1);
    auto it = roiKeyValues.find("ROI_NAME");
    ASSERT_TRUE(it != roiKeyValues.end());
    EXPECT_EQ(it->second, "S1");
    ASSERT_EQ(roiPoints.size(), 3);
    EXPECT_EQ(roiPoints[0], cv::Point(120, 80));
    EXPECT_EQ(roiPoints[1], cv::Point(520, 80));
    EXPECT_EQ(roiPoints[2], cv::Point(520, 380));
  }
  {
    const auto& [roiKeyValues, roiPoints] = roiPointsList[1];
    ASSERT_EQ(roiKeyValues.size(), 1);
    auto it = roiKeyValues.find("ROI_NAME");
    ASSERT_TRUE(it != roiKeyValues.end());
    EXPECT_EQ(it->second, "SLOT2");
    ASSERT_EQ(roiPoints.size(), 4);
    EXPECT_EQ(roiPoints[0], cv::Point(640, 200));
    EXPECT_EQ(roiPoints[1], cv::Point(940, 200));
    EXPECT_EQ(roiPoints[2], cv::Point(940, 600));
    EXPECT_EQ(roiPoints[3], cv::Point(640, 600));
  }
}

TEST(XMLTest, ROI_RECT) {
  std::string xmlRois = R"(
    <MARKED_ROI id="SLOTS">
      <NUM_ROI>3</NUM_ROI>
      <ROI id="0">
        <ROI_NAME>SLOT1</ROI_NAME>
        <MARK_POINT>
          <POINTS>4</POINTS>
          <P1> 120,80 </P1>
          <P2> 520,80 </P2>
          <P3> 520,380 </P3>
          <P4> 120,380 </P4>
        </MARK_POINT>
      </ROI>
      <ROI id="1">
        <ROI_NAME>SLOT2</ROI_NAME>
        <MARK_POINT>
          <POINTS>4</POINTS>
          <P1> 640,200 </P1>
          <P2> 940,200 </P2>
          <P3> 940,600 </P3>
          <P4> 640,600 </P4>
        </MARK_POINT>
      </ROI>
      <ROI id="2">
        <ROI_NAME>SLOT3</ROI_NAME>
        <MARK_POINT>
          <POINTS>4</POINTS>
          <P1> 50,500 </P1>
          <P2> 250,500 </P2>
          <P3> 250,750 </P3>
          <P4> 50,750 </P4>
        </MARK_POINT>
      </ROI>
    </MARKED_ROI>
  )";

  auto [markedPointAttributes, roiRectList] = Parsers::KeyValueParser::parseValue<Parsers::KeyValueParser::ROI, Parsers::KeyValueParser::XML, Parsers::MarkedROIRects>(xmlRois);
    ASSERT_EQ(markedPointAttributes.size(), 1);
  ASSERT_TRUE(markedPointAttributes.find("id") != markedPointAttributes.end());
  auto id = markedPointAttributes.at("id");
  EXPECT_EQ(id, "SLOTS");
  ASSERT_EQ(roiRectList.size(), 3);
  {
    const auto& [roiKeyValues, roiRect] = roiRectList[0];
    ASSERT_EQ(roiKeyValues.size(), 1);
    auto it = roiKeyValues.find("ROI_NAME");
    ASSERT_TRUE(it != roiKeyValues.end());
    EXPECT_EQ(it->second, "SLOT1");
    EXPECT_EQ(roiRect, cv::Rect(120, 80, 400, 300));
  }
  {
    const auto& [roiKeyValues, roiRect] = roiRectList[1];
    ASSERT_EQ(roiKeyValues.size(), 1);
    auto it = roiKeyValues.find("ROI_NAME");
    ASSERT_TRUE(it != roiKeyValues.end());
    EXPECT_EQ(it->second, "SLOT2");
    EXPECT_EQ(roiRect, cv::Rect(640, 200, 300, 400));
  }
  {
    const auto& [roiKeyValues, roiRect] = roiRectList[2];
    ASSERT_EQ(roiKeyValues.size(), 1);
    auto it = roiKeyValues.find("ROI_NAME");
    ASSERT_TRUE(it != roiKeyValues.end());
    EXPECT_EQ(it->second, "SLOT3");
    EXPECT_EQ(roiRect, cv::Rect(50, 500, 200, 250));
  }
}

// JSON Tests

TEST(JsonTest, POINTS)
{
  json jsonPoints = json::array();
  jsonPoints.push_back({{"x", 296}, {"y", 1015}});
  jsonPoints.push_back({{"x", 916}, {"y", 0}});
  jsonPoints.push_back({{"x", 1096}, {"y", 32}});

  json markPoints = { {"MARK_POINT", jsonPoints} };
  auto points = Parsers::KeyValueParser::parseValue<Parsers::KeyValueParser::ROI, Parsers::KeyValueParser::JSON, std::vector<cv::Point>>(markPoints.dump());
  ASSERT_EQ(points.size(), 3);
  EXPECT_EQ(points[0], cv::Point(296, 1015));
  EXPECT_EQ(points[1], cv::Point(916, 0));
  EXPECT_EQ(points[2], cv::Point(1096, 32));
}

TEST(JsonTest, ROI_POINTS) {
  json rois = json::array();
  rois.push_back({
    {"ROI_NAME", "S1"},
    {"MARK_POINT", {
      {{"x", 120}, {"y", 80}},   // top-left
      {{"x", 520}, {"y", 80}},   // top-right
      {{"x", 520}, {"y", 380}},  // bottom-right
    }}
  });
  rois.push_back({
    {"ROI_NAME", "SLOT2"},
    {"MARK_POINT", {
      {{"x", 640}, {"y", 200}},   // top-left
      {{"x", 940}, {"y", 200}},   // top-right
      {{"x", 940}, {"y", 600}},   // bottom-right
      {{"x", 640}, {"y", 600}}    // bottom-left
    }}
  });

  json marked_roi = {
    {"MARKED_ROI", {
      {"id", "SLOTS"},
      {"value", rois}
    }}
  };

  auto [markedPointAttributes, roiPointsList] = Parsers::KeyValueParser::parseValue<Parsers::KeyValueParser::ROI, Parsers::KeyValueParser::JSON, Parsers::MarkedROIPoints>(marked_roi.dump());
  ASSERT_EQ(markedPointAttributes.size(), 1);
  ASSERT_TRUE(markedPointAttributes.find("id") != markedPointAttributes.end());
  auto id = markedPointAttributes.at("id");
  EXPECT_EQ(id, "SLOTS");
  ASSERT_EQ(roiPointsList.size(), 2);
  {
    const auto& [roiKeyValues, roiPoints] = roiPointsList[0];
    ASSERT_EQ(roiKeyValues.size(), 1);
    auto it = roiKeyValues.find("ROI_NAME");
    ASSERT_TRUE(it != roiKeyValues.end());
    EXPECT_EQ(it->second, "S1");
    ASSERT_EQ(roiPoints.size(), 3);
    EXPECT_EQ(roiPoints[0], cv::Point(120, 80));
    EXPECT_EQ(roiPoints[1], cv::Point(520, 80));
    EXPECT_EQ(roiPoints[2], cv::Point(520, 380));
  }
  {
    const auto& [roiKeyValues, roiPoints] = roiPointsList[1];
    ASSERT_EQ(roiKeyValues.size(), 1);
    auto it = roiKeyValues.find("ROI_NAME");
    ASSERT_TRUE(it != roiKeyValues.end());
    EXPECT_EQ(it->second, "SLOT2");
    ASSERT_EQ(roiPoints.size(), 4);
    EXPECT_EQ(roiPoints[0], cv::Point(640, 200));
    EXPECT_EQ(roiPoints[1], cv::Point(940, 200));
    EXPECT_EQ(roiPoints[2], cv::Point(940, 600));
    EXPECT_EQ(roiPoints[3], cv::Point(640, 600));
  }
}

TEST(JsonTest, ROI_RECT) {
  json rois = json::array();
  // ROI 1
  rois.push_back({
      {"ROI_NAME", "SLOT1"},
      {"MARK_POINT", {
          {{"x", 120}, {"y", 80}},   // top-left
          {{"x", 520}, {"y", 80}},   // top-right
          {{"x", 520}, {"y", 380}},  // bottom-right
          {{"x", 120}, {"y", 380}}   // bottom-left
      }}
  });

  // ROI 2
  rois.push_back({
      {"ROI_NAME", "SLOT2"},
      {"MARK_POINT", {
          {{"x", 640}, {"y", 200}},   // top-left
          {{"x", 940}, {"y", 200}},   // top-right
          {{"x", 940}, {"y", 600}},   // bottom-right
          {{"x", 640}, {"y", 600}}    // bottom-left
      }}
  });

  // ROI 3
  rois.push_back({
      {"ROI_NAME", "SLOT3"},
      {"MARK_POINT", {
          {{"x", 50}, {"y", 500}},    // top-left
          {{"x", 250}, {"y", 500}},   // top-right
          {{"x", 250}, {"y", 750}},   // bottom-right
          {{"x", 50}, {"y", 750}}     // bottom-left
      }}
  });

  json marked_roi = {
    {"MARKED_ROI", {
      {"id", "SLOTS"},
      {"value", rois}
    }}
  };

  auto [markedPointAttributes, roiRectList] = Parsers::KeyValueParser::parseValue<Parsers::KeyValueParser::ROI, Parsers::KeyValueParser::JSON, Parsers::MarkedROIRects>(marked_roi.dump());
    ASSERT_EQ(markedPointAttributes.size(), 1);
  ASSERT_TRUE(markedPointAttributes.find("id") != markedPointAttributes.end());
  auto id = markedPointAttributes.at("id");
  EXPECT_EQ(id, "SLOTS");
  ASSERT_EQ(roiRectList.size(), 3);
  {
    const auto& [roiKeyValues, roiRect] = roiRectList[0];
    ASSERT_EQ(roiKeyValues.size(), 1);
    auto it = roiKeyValues.find("ROI_NAME");
    ASSERT_TRUE(it != roiKeyValues.end());
    EXPECT_EQ(it->second, "SLOT1");
    EXPECT_EQ(roiRect, cv::Rect(120, 80, 400, 300));
  }
  {
    const auto& [roiKeyValues, roiRect] = roiRectList[1];
    ASSERT_EQ(roiKeyValues.size(), 1);
    auto it = roiKeyValues.find("ROI_NAME");
    ASSERT_TRUE(it != roiKeyValues.end());
    EXPECT_EQ(it->second, "SLOT2");
    EXPECT_EQ(roiRect, cv::Rect(640, 200, 300, 400));
  }
  {
    const auto& [roiKeyValues, roiRect] = roiRectList[2];
    ASSERT_EQ(roiKeyValues.size(), 1);
    auto it = roiKeyValues.find("ROI_NAME");
    ASSERT_TRUE(it != roiKeyValues.end());
    EXPECT_EQ(it->second, "SLOT3");
    EXPECT_EQ(roiRect, cv::Rect(50, 500, 200, 250));
  }
}
