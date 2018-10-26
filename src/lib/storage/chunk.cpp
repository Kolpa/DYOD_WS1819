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

void Chunk::add_segment(std::shared_ptr<BaseSegment> segment) { _columns.push_back(segment); }

void Chunk::append(const std::vector<AllTypeVariant>& values) {
  DebugAssert(values.size() == _columns.size(),
              "Number of passed arguments does not equal the number of stored columns.");
  auto value_iter = values.cbegin();
  for (auto const& column : _columns) {
    column->append(*value_iter);
    ++value_iter;
  }
}

std::shared_ptr<BaseSegment> Chunk::get_segment(ColumnID column_id) const {
  if (_columns.size() > column_id.t) {
    return _columns[column_id.t];
  }
  return nullptr;
}

uint16_t Chunk::column_count() const { return _columns.size(); }

uint32_t Chunk::size() const {
  if (_columns.empty()) {
    return 0;
  } else {
    return _columns[0]->size();
  }
}

}  // namespace opossum
