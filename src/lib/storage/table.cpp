#include "table.hpp"

#include <shared_mutex>

#include <algorithm>
#include <iomanip>
#include <limits>
#include <memory>
#include <mutex>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include "dictionary_segment.hpp"
#include "value_segment.hpp"

#include "resolve_type.hpp"
#include "types.hpp"

namespace opossum {

Table::Table(const uint32_t chunk_size)
    : _chunk_size{chunk_size}, _current_chunk{std::make_shared<Chunk>()}, _chunks{_current_chunk} {}

void Table::add_column_definition(const std::string& name, const std::string& type) {
  DebugAssert(_column_names.size() < std::numeric_limits<uint16_t>::max(), "Maximum amount of columns reached.");
  DebugAssert(row_count() == 0, "Cannot add columns if table contains any rows.");

  _column_names.push_back(name);
  _column_types.push_back(type);
}

void Table::add_column(const std::string& name, const std::string& type) {
  add_column_definition(name, type);

  const auto segment = make_shared_by_data_type<BaseSegment, ValueSegment>(type);
  _current_chunk->add_segment(segment);
}

void Table::append(std::vector<AllTypeVariant> values) {
  if (_is_full(*_current_chunk)) {
    create_new_chunk();
  }

  _current_chunk->append(values);
}

// creates a new chunk and sets it as current chunk;
void Table::create_new_chunk() {
  _current_chunk = std::make_shared<Chunk>();
  for (const auto& type : _column_types) {
    const auto segment = make_shared_by_data_type<BaseSegment, ValueSegment>(type);
    _current_chunk->add_segment(segment);
  }
  _chunks.push_back(_current_chunk);
}

uint16_t Table::column_count() const { return static_cast<uint16_t>(_column_names.size()); }

uint64_t Table::row_count() const {
  uint64_t rows{0};
  for (const auto& chunk : _chunks) {
    rows += chunk->size();
  }
  return rows;
}

ChunkID Table::chunk_count() const { return static_cast<ChunkID>(_chunks.size()); }

ColumnID Table::column_id_by_name(const std::string& column_name) const {
  const auto column_name_iter = std::find(_column_names.cbegin(), _column_names.cend(), column_name);
  DebugAssert(column_name_iter != _column_names.cend(), "A column with the passed column name does not exist.");
  const uint16_t position = std::distance(_column_names.cbegin(), column_name_iter);
  return ColumnID(position);
}

uint32_t Table::chunk_size() const { return _chunk_size; }

const std::vector<std::string>& Table::column_names() const { return _column_names; }

const std::string& Table::column_name(ColumnID column_id) const {
  DebugAssert(column_id < _column_names.size(), "Column id is out of bounds.");
  return _column_names[column_id];
}

const std::string& Table::column_type(ColumnID column_id) const {
  DebugAssert(column_id < _column_types.size(), "Column id is out of bounds.");
  return _column_types[column_id];
}

bool Table::_is_full(const Chunk& chunk) const { return chunk.size() == _chunk_size; }

void Table::emplace_chunk(Chunk chunk) {
  DebugAssert(chunk.size() <= _chunk_size, "The chunk's size must not exceed the maximum chunk size of the table.");

  if (row_count() == 0) {
    _current_chunk = std::make_shared<Chunk>(std::move(chunk));
    _chunks[0] = _current_chunk;
    return;
  }

  // This is just a primitive verification
  DebugAssert(chunk.column_count() == column_count(), "The chunk's columns must match to the columns of the table.");
  _current_chunk = std::make_shared<Chunk>(std::move(chunk));
  _chunks.push_back(_current_chunk);
}

Chunk& Table::get_chunk(ChunkID chunk_id) {
  DebugAssert(chunk_id < _chunks.size(), "Chunk id is out of bound.");
  std::shared_lock lock(_chunks_mutex);
  return *_chunks[chunk_id];
}

const Chunk& Table::get_chunk(ChunkID chunk_id) const { return const_cast<Table*>(this)->get_chunk(chunk_id); }

// compresses the chunk compressing the value_segments to dictionary_segments.
// The chunk to be compressed must be full.
void Table::compress_chunk(ChunkID chunk_id) {
  const auto& uncompressed_chunk = get_chunk(chunk_id);

  DebugAssert(_is_full(uncompressed_chunk), "Chunk to compress must be full.");

  auto compressed_chunk = std::make_shared<Chunk>();
  for (ColumnID column_id{0}; column_id < uncompressed_chunk.column_count(); ++column_id) {
    const auto segment = uncompressed_chunk.get_segment(column_id);
    const auto dictionary_segment =
        make_shared_by_data_type<BaseSegment, DictionarySegment>(column_type(column_id), segment);
    compressed_chunk->add_segment(dictionary_segment);
  }

  // Replace uncompressed chunk with dictionary compressed chunk.
  std::lock_guard lock(_chunks_mutex);
  _chunks[chunk_id] = std::move(compressed_chunk);
}

}  // namespace opossum
