#include <algorithm>
#include <limits>
#include <string>
#include <vector>

#include "../base_test.hpp"
#include "gtest/gtest.h"

#include "../lib/storage/value_segment.hpp"

namespace opossum {

class StorageValueSegmentTest : public BaseTest {
 protected:
  ValueSegment<int> int_value_segment;
  ValueSegment<std::string> string_value_segment;
  ValueSegment<double> double_value_segment;
};

TEST_F(StorageValueSegmentTest, GetSize) {
  EXPECT_EQ(int_value_segment.size(), 0u);
  EXPECT_EQ(string_value_segment.size(), 0u);
  EXPECT_EQ(double_value_segment.size(), 0u);
}

TEST_F(StorageValueSegmentTest, AddValueOfSameType) {
  int_value_segment.append(3);
  EXPECT_EQ(int_value_segment.size(), 1u);

  string_value_segment.append("Hello");
  EXPECT_EQ(string_value_segment.size(), 1u);

  double_value_segment.append(3.14);
  EXPECT_EQ(double_value_segment.size(), 1u);
}

TEST_F(StorageValueSegmentTest, AddValueOfDifferentType) {
  int_value_segment.append(3.14);
  EXPECT_EQ(int_value_segment.size(), 1u);
  EXPECT_THROW(int_value_segment.append("Hi"), std::exception);

  string_value_segment.append(3);
  string_value_segment.append(4.44);
  EXPECT_EQ(string_value_segment.size(), 2u);

  double_value_segment.append(4);
  EXPECT_EQ(double_value_segment.size(), 1u);
  EXPECT_THROW(double_value_segment.append("Hi"), std::exception);
}

TEST_F(StorageValueSegmentTest, DirectValueIndexing) {
  int_value_segment.append(3);
  EXPECT_EQ(type_cast<int>(int_value_segment[0]), 3);
  int_value_segment.append(42);
  EXPECT_EQ(type_cast<int>(int_value_segment[1]), 42);

  string_value_segment.append("Develop");
  EXPECT_EQ(type_cast<std::string>(string_value_segment[0]), "Develop");
  string_value_segment.append("Your");
  EXPECT_EQ(type_cast<std::string>(string_value_segment[1]), "Your");

  double_value_segment.append(3.14);
  EXPECT_EQ(type_cast<double>(double_value_segment[0]), 3.14);
  double_value_segment.append(4.44);
  EXPECT_EQ(type_cast<double>(double_value_segment[1]), 4.44);
}

TEST_F(StorageValueSegmentTest, GetValues) {
  std::vector<int> numbers{1, 4, 4, 8, 0};

  for (const auto n : numbers) {
    int_value_segment.append(n);
  }

  EXPECT_TRUE(std::equal(numbers.cbegin(), numbers.cend(), int_value_segment.values().cbegin()));
}

}  // namespace opossum
