#include <memory>
#include <string>

#include "../base_test.hpp"
#include "gtest/gtest.h"

#include "../lib/resolve_type.hpp"
#include "../lib/storage/base_segment.hpp"
#include "../lib/storage/chunk.hpp"
#include "../lib/types.hpp"

namespace opossum {

class StorageChunkTest : public BaseTest {
 protected:
  void SetUp() override {
    int_value_segment = make_shared_by_data_type<BaseSegment, ValueSegment>("int");
    int_value_segment->append(4);
    int_value_segment->append(6);
    int_value_segment->append(3);

    string_value_segment = make_shared_by_data_type<BaseSegment, ValueSegment>("string");
    string_value_segment->append("Hello,");
    string_value_segment->append("world");
    string_value_segment->append("!");
  }

  Chunk c;
  std::shared_ptr<BaseSegment> int_value_segment = nullptr;
  std::shared_ptr<BaseSegment> string_value_segment = nullptr;
};

TEST_F(StorageChunkTest, AddSegmentToChunk) {
  EXPECT_EQ(c.size(), 0u);
  c.add_segment(int_value_segment);
  EXPECT_EQ(c.size(), 3u);
  c.add_segment(string_value_segment);
  EXPECT_EQ(c.size(), 3u);
}

TEST_F(StorageChunkTest, AddValuesToChunk) {
  c.add_segment(int_value_segment);
  c.add_segment(string_value_segment);
  c.append({2, "two"});
  EXPECT_EQ(c.size(), 4u);

  if (IS_DEBUG) {
    EXPECT_THROW(c.append({}), std::exception);
    EXPECT_THROW(c.append({4, "val", 3}), std::exception);
    EXPECT_EQ(c.size(), 4u);
  }
}

TEST_F(StorageChunkTest, RetrieveSegment) {
  c.add_segment(int_value_segment);
  c.add_segment(string_value_segment);
  c.append({2, "two"});

  auto base_segment = c.get_segment(ColumnID{0});
  EXPECT_EQ(base_segment->size(), 4u);
  EXPECT_EQ(type_cast<int>((*base_segment)[3]), 2);

  base_segment = c.get_segment(ColumnID{1});
  EXPECT_EQ(type_cast<std::string>((*base_segment)[3]), "two");
}

TEST_F(StorageChunkTest, UnknownSegmentType) {
  // Exception will only be thrown in debug builds
  if (IS_DEBUG) {
    auto wrapper = []() { make_shared_by_data_type<BaseSegment, ValueSegment>("weird_type"); };
    EXPECT_THROW(wrapper(), std::logic_error);
  }
}

TEST_F(StorageChunkTest, GetColumnCount) {
  EXPECT_EQ(c.column_count(), 0u);
  c.add_segment(int_value_segment);
  EXPECT_EQ(c.column_count(), 1u);
  c.add_segment(int_value_segment);
  EXPECT_EQ(c.column_count(), 2u);
}

}  // namespace opossum
