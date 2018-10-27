#include "table.hpp"

#include <algorithm>
#include <iomanip>
#include <limits>
#include <memory>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include "value_segment.hpp"

#include "resolve_type.hpp"
#include "types.hpp"
#include "utils/assert.hpp"

namespace opossum {

Table::Table(const uint32_t chunk_size) {
  _chunk_size = chunk_size;
  auto chunk = std::make_shared<Chunk>();
  _chunks.push_back(chunk);
}

void Table::add_column(const std::string& name, const std::string& type) {
  if (_column_names.size() < std::numeric_limits<uint16_t>::max()) {
    _column_names.push_back(name);
    _column_types.push_back(type);
    auto segment = make_shared_by_data_type<BaseSegment, ValueSegment>(type);
    _chunks.back()->add_segment(segment);
  }
}

void Table::append(std::vector<AllTypeVariant> values) {
  _chunks.back().get()->append(values);
  if (_chunks.back().get()->size() == _chunk_size) {
    auto chunk = std::make_shared<Chunk>();
    for(auto const& type: _column_types){
      auto segment = make_shared_by_data_type<BaseSegment, ValueSegment>(type);
      chunk->add_segment(segment);
    }
    _chunks.push_back(chunk);
  }
}

uint16_t Table::column_count() const { return static_cast<uint16_t>(_column_names.size()); }

uint64_t Table::row_count() const {
  uint64_t number_of_rows = 0;
  for (auto const& chunk : _chunks) {
    number_of_rows += chunk->size();
  }
  return number_of_rows;
}

ChunkID Table::chunk_count() const { return ChunkID{static_cast<uint16_t>(_chunks.size())}; }

ColumnID Table::column_id_by_name(const std::string& column_name) const {
  auto iterator = std::find(_column_names.begin(), _column_names.end(), column_name);
  if (iterator != _column_names.end()) {
    uint16_t position = std::distance(_column_names.begin(), iterator);
    return ColumnID(position);
  } else {
    throw std::invalid_argument("A column with the passed column name does not exist.");
  }
}

uint32_t Table::chunk_size() const { return _chunk_size; }

const std::vector<std::string>& Table::column_names() const { return _column_names; }

const std::string& Table::column_name(ColumnID column_id) const { return _column_names[column_id]; }

const std::string& Table::column_type(ColumnID column_id) const { return _column_types[column_id]; }

Chunk& Table::get_chunk(ChunkID chunk_id) { return *_chunks[chunk_id]; }

const Chunk& Table::get_chunk(ChunkID chunk_id) const { return get_chunk(chunk_id); }

}  // namespace opossum
