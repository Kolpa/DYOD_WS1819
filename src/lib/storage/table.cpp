#include "table.hpp"

#include <algorithm>
#include <iomanip>
#include <limits>
#include <memory>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include "dictionary_segment.hpp"
#include "value_segment.hpp"

#include "resolve_type.hpp"
#include "types.hpp"
#include "utils/assert.hpp"

namespace opossum {

Table::Table(const uint32_t chunk_size)
    : _chunk_size{chunk_size}, _current_chunk{std::make_shared<Chunk>()}, _chunks{_current_chunk} {}

void Table::add_column(const std::string& name, const std::string& type) {
  DebugAssert(_column_names.size() < std::numeric_limits<uint16_t>::max(), "Maximum amount of columns reached.");
  DebugAssert(!row_count(), "Cannot add columns if table contains any rows.");

  _column_names.push_back(name);
  _column_types.push_back(type);

  const auto segment = make_shared_by_data_type<BaseSegment, ValueSegment>(type);
  _current_chunk->add_segment(segment);
}

void Table::append(std::vector<AllTypeVariant> values) {
  // check if chunk is full
  if (_is_full(*_current_chunk)) {
    _open_new_chunk();
  }

  _current_chunk->append(values);
}

void Table::_open_new_chunk() {
  _current_chunk = std::make_shared<Chunk>();
  for (const auto& type : _column_types) {
    const auto segment = make_shared_by_data_type<BaseSegment, ValueSegment>(type);
    _current_chunk->add_segment(segment);
  }
  _chunks.push_back(_current_chunk);
}

uint16_t Table::column_count() const { return static_cast<uint16_t>(_column_names.size()); }

uint64_t Table::row_count() const {
  return static_cast<uint64_t>((_chunks.size() - 1) * _chunk_size + _current_chunk->size());
}

ChunkID Table::chunk_count() const { return ChunkID{static_cast<uint16_t>(_chunks.size())}; }

ColumnID Table::column_id_by_name(const std::string& column_name) const {
  const auto column_name_it = std::find(_column_names.cbegin(), _column_names.cend(), column_name);
  DebugAssert(column_name_it != _column_names.cend(), "A column with the passed column name does not exist.");
  const uint16_t position = std::distance(_column_names.cbegin(), column_name_it);
  return ColumnID(position);
}

uint32_t Table::chunk_size() const { return _chunk_size; }

const std::vector<std::string>& Table::column_names() const { return _column_names; }

const std::string& Table::column_name(ColumnID column_id) const { return _column_names[column_id]; }

const std::string& Table::column_type(ColumnID column_id) const { return _column_types[column_id]; }

Chunk& Table::get_chunk(ChunkID chunk_id) { return *_chunks[chunk_id]; }

bool Table::_is_full(const Chunk& chunk) const { return chunk.size() == _chunk_size; }

void Table::emplace_chunk(Chunk chunk) {
  DebugAssert(chunk.size() <= _chunk_size, "The chunk's size must not exceed the maximum chunk size of the table.");

  if (row_count() == 0) {
    _current_chunk = std::make_shared<Chunk>(std::move(chunk));
    _chunks[0] = _current_chunk;
    return;
  }

  DebugAssert(_is_full(*_current_chunk), "The last chunk must be completely filled.");
  // This is just a primitive verification
  DebugAssert(chunk.column_count() == column_count(), "The chunk's columns must match to the columns of the table.");
  _current_chunk = std::make_shared<Chunk>(std::move(chunk));
  _chunks.push_back(_current_chunk);
}

const Chunk& Table::get_chunk(ChunkID chunk_id) const { return get_chunk(chunk_id); }

void Table::compress_chunk(ChunkID chunk_id) {
  // 0. neuen Chunk erzeugen - easy
  Chunk compressed_chunk;

  // 1. chunk holen -- get chunk
  const auto& uncompressed_chunk = get_chunk(chunk_id);

  // 1.1 Segmente über get segment holen
  for (ColumnID column_id{0}; column_id < uncompressed_chunk.column_count(); ++column_id) {
    const auto segment = uncompressed_chunk.get_segment(column_id);
    // 2. Segmente komprimieren - erfolgt im Konstruktor des Dictionary segments
    const auto dictionary_segment =
        make_shared_by_data_type<BaseSegment, DictionarySegment>(column_type(column_id), segment);
    // 4. Segmente zu neuem Chunk hinzufügen - chunk.add_segment
    compressed_chunk.add_segment(dictionary_segment);
  }

  // TODO: Mutex auf Chunk
  // 5. Chunk ersetzen // Nebenläufigkeit beachten _chunks[chunk_id] = new_chunk.
  _chunks[chunk_id] = std::make_shared<Chunk>(std::move(compressed_chunk));
}

}  // namespace opossum
