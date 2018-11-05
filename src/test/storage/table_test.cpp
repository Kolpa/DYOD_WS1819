#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../base_test.hpp"
#include "gtest/gtest.h"

#include "../lib/resolve_type.hpp"
#include "../lib/storage/dictionary_segment.hpp"
#include "../lib/storage/table.hpp"
#include "../lib/types.hpp"

namespace opossum {

class StorageTableTest : public BaseTest {
 protected:
  void SetUp() override {
    t.add_column("col_1", "int");
    t.add_column("col_2", "string");
  }

  Table t{2};
};

TEST_F(StorageTableTest, AddColumn) {
  Table table;
  EXPECT_EQ(table.column_count(), 0u);
  table.add_column("a", "int");
  EXPECT_EQ(table.column_count(), 1u);
  table.append({0});
  EXPECT_EQ(table.column_count(), 1u);
  EXPECT_THROW(table.add_column("b", "int"), std::exception);
}

TEST_F(StorageTableTest, ChunkCount) {
  EXPECT_EQ(t.chunk_count(), 1u);
  t.append({4, "Hello,"});
  t.append({6, "world"});
  t.append({3, "!"});
  EXPECT_EQ(t.chunk_count(), 2u);
}

TEST_F(StorageTableTest, GetChunk) {
  t.get_chunk(ChunkID{0});
  // TODO(anyone): Do we want checks here?
  // EXPECT_THROW(t.get_chunk(ChunkID{q}), std::exception);
  t.append({4, "Hello,"});
  t.append({6, "world"});
  t.append({3, "!"});
  t.get_chunk(ChunkID{1});
}

TEST_F(StorageTableTest, EmplaceChunkOnNonEmptyTable) {
  Chunk c;
  c.add_segment(make_shared_by_data_type<BaseSegment, ValueSegment>("int"));
  c.add_segment(make_shared_by_data_type<BaseSegment, ValueSegment>("string"));
  t.append({1, "DYOD"});

  // Fail, because the last chunk is not completely full
  EXPECT_THROW(t.emplace_chunk(std::move(c)), std::exception);

  t.append({2, "DYOD"});
  EXPECT_EQ(t.chunk_count(), 1u);
  c.add_segment(make_shared_by_data_type<BaseSegment, ValueSegment>("int"));
  c.add_segment(make_shared_by_data_type<BaseSegment, ValueSegment>("string"));
  // line should succeed.
  t.emplace_chunk(std::move(c));
  EXPECT_EQ(t.chunk_count(), 2u);

  // fill emplaced chunk
  t.append({3, "DYOD"});
  t.append({4, "DYOD"});
  EXPECT_EQ(t.chunk_count(), 2u);

  c.add_segment(make_shared_by_data_type<BaseSegment, ValueSegment>("int"));
  // fail, because chunk has wrong number of segments
  EXPECT_THROW(t.emplace_chunk(std::move(c)), std::exception);
}

TEST_F(StorageTableTest, EmplaceChunkOnEmptyTable) {
  Chunk c;
  // Initially the table has one chunk
  EXPECT_EQ(t.chunk_count(), 1u);

  t.emplace_chunk(std::move(c));
  // after having emplaced another chunk, the table should still have one chunk.
  EXPECT_EQ(t.chunk_count(), 1u);

  // The chunk has no segments. Appending a row should fail.
  EXPECT_THROW(t.append({42, "DYOD", 3.14}), std::exception);

  c.add_segment(make_shared_by_data_type<BaseSegment, ValueSegment>("int"));
  c.add_segment(make_shared_by_data_type<BaseSegment, ValueSegment>("string"));
  c.add_segment(make_shared_by_data_type<BaseSegment, ValueSegment>("double"));
  t.emplace_chunk(std::move(c));
  // we replaced the chunk with no segments by a chunk with 3 segments. The following statement should succeed.
  t.append({42, "DYOD", 3.14});

  // since we now have a row in our table and
  EXPECT_THROW(t.emplace_chunk(std::move(c)), std::exception);
}

TEST_F(StorageTableTest, EmplaceTooBigChunk) {
  Chunk c;

  c.add_segment(make_shared_by_data_type<BaseSegment, ValueSegment>("int"));
  c.add_segment(make_shared_by_data_type<BaseSegment, ValueSegment>("string"));
  c.add_segment(make_shared_by_data_type<BaseSegment, ValueSegment>("double"));

  c.append({42, "Develop", 3.14});
  c.append({42, "Your", 3.14});
  c.append({42, "Own", 3.14});

  // fail because chunk_size is bigger than chunk_size of table.
  EXPECT_THROW(t.emplace_chunk(std::move(c)), std::exception);
}

TEST_F(StorageTableTest, ColumnCount) { EXPECT_EQ(t.column_count(), 2u); }

TEST_F(StorageTableTest, RowCount) {
  EXPECT_EQ(t.row_count(), 0u);
  t.append({4, "Hello,"});
  t.append({6, "world"});
  t.append({3, "!"});
  EXPECT_EQ(t.row_count(), 3u);
}

TEST_F(StorageTableTest, GetColumnName) {
  EXPECT_EQ(t.column_name(ColumnID{0}), "col_1");
  EXPECT_EQ(t.column_name(ColumnID{1}), "col_2");
  // TODO(anyone): Do we want checks here?
  // EXPECT_THROW(t.column_name(ColumnID{2}), std::exception);
}

TEST_F(StorageTableTest, GetColumnNames) {
  EXPECT_EQ(t.column_names().size(), 2u);
  EXPECT_EQ(t.column_names()[0], "col_1");
  EXPECT_EQ(t.column_names()[1], "col_2");
}

TEST_F(StorageTableTest, GetColumnType) {
  EXPECT_EQ(t.column_type(ColumnID{0}), "int");
  EXPECT_EQ(t.column_type(ColumnID{1}), "string");
  // TODO(anyone): Do we want checks here?
  // EXPECT_THROW(t.column_type(ColumnID{2}), std::exception);
}

TEST_F(StorageTableTest, GetColumnIdByName) {
  EXPECT_EQ(t.column_id_by_name("col_2"), 1u);
  EXPECT_THROW(t.column_id_by_name("no_column_name"), std::exception);
}

TEST_F(StorageTableTest, GetChunkSize) {
  EXPECT_EQ(t.chunk_size(), 2u);

  Table table;
  EXPECT_EQ(table.chunk_size(), std::numeric_limits<ChunkOffset>::max() - 1);
}

TEST_F(StorageTableTest, Append) {
  EXPECT_EQ(t.row_count(), 0u);
  t.append({42, "DY"});
  t.append({43, "OD"});
  EXPECT_EQ(t.row_count(), 2u);
  EXPECT_EQ(type_cast<int>((*t.get_chunk(ChunkID{0}).get_segment(ColumnID{0}))[0]), 42);
}

TEST_F(StorageTableTest, CompressChunk) {
  t.append({1, "v1"});
  t.append({2, "v2"});
  t.append({3, "v3"});
  t.append({4, "v4"});
  t.append({5, "v5"});
  t.append({6, "v6"});
  t.append({7, "v7"});

  EXPECT_EQ(t.chunk_count(), 4u);

  auto segment = t.get_chunk(ChunkID{1}).get_segment(ColumnID{0});
  auto value_segment_ptr = std::dynamic_pointer_cast<ValueSegment<int>>(segment);
  auto dictionary_segment_ptr = std::dynamic_pointer_cast<DictionarySegment<int>>(segment);
  EXPECT_TRUE(value_segment_ptr != nullptr);
  EXPECT_TRUE(dictionary_segment_ptr == nullptr);

  t.compress_chunk(ChunkID{1});

  segment = t.get_chunk(ChunkID{1}).get_segment(ColumnID{0});
  value_segment_ptr = std::dynamic_pointer_cast<ValueSegment<int>>(segment);
  dictionary_segment_ptr = std::dynamic_pointer_cast<DictionarySegment<int>>(segment);
  EXPECT_TRUE(value_segment_ptr == nullptr);
  EXPECT_TRUE(dictionary_segment_ptr != nullptr);

  EXPECT_EQ(t.chunk_count(), 4u);

  // Fail because last chunk not full.
  EXPECT_THROW(t.compress_chunk(ChunkID{t.chunk_count() - 1}), std::exception);
}

}  // namespace opossum
