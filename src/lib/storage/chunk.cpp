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
  for (std::size_t index = 0; index < values.size(); ++index) {
    _columns.at(index).get()->append(values.at(index));
  }
}

std::shared_ptr<BaseSegment> Chunk::get_segment(ColumnID column_id) const {
  if (_columns.size() > column_id.t) {
    return _columns.at(column_id.t);
  }
  return nullptr;
}

uint16_t Chunk::column_count() const { return _columns.size(); }

uint32_t Chunk::size() const {
  if (_columns.size() == 0) {
    return 0;
  } else {
    return _columns.at(0).get()->size();
  }
}

}  // namespace opossum
