#include <iomanip>
#include <iterator>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "base_segment.hpp"
#include "chunk.hpp"

#include "utils/assert.hpp"

namespace opossum {

void Chunk::add_segment(std::shared_ptr<BaseSegment> segment) { _segments.push_back(segment); }

void Chunk::append(const std::vector<AllTypeVariant>& values) {
  DebugAssert(values.size() == _segments.size(),
              "Number of passed arguments does not equal the number of stored columns.");
  auto value_iter = values.cbegin();
  for (const auto& segment : _segments) {
    segment->append(*value_iter);
    ++value_iter;
  }
}

std::shared_ptr<BaseSegment> Chunk::get_segment(ColumnID column_id) const { return _segments[column_id]; }

uint16_t Chunk::column_count() const { return _segments.size(); }

uint32_t Chunk::size() const {
  if (_segments.empty()) {
    return 0;
  } else {
    return _segments[0]->size();
  }
}

}  // namespace opossum
