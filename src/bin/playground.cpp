#include <storage/dictionary_segment.hpp>
#include <storage/table.hpp>
#include <storage/value_segment.hpp>

#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "../lib/utils/assert.hpp"

int main() {
  opossum::Assert(true, "We can use opossum files here :)");

  opossum::Table t(2);
  t.add_column("col_1", "string");
  t.add_column("col_2", "int");

  t.append({"v1", 1});
  t.append({"v2", 2});
  t.append({"v3", 3});
  t.append({"v4", 4});
  t.append({"v5", 5});
  t.append({"v6", 6});
  t.append({"v7", 7});

  std::cout << t.chunk_size() << std::endl;
  std::cout << t.chunk_count() << std::endl;
  t.compress_chunk(opossum::ChunkID{1});
  // exception expected
  t.compress_chunk(opossum::ChunkID{t.chunk_count() - 1});

  return 0;
}
