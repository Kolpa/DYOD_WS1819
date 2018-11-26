#include "reference_segment.hpp"

#include <memory>

#include "utils/assert.hpp"

namespace opossum {

ReferenceSegment::ReferenceSegment(const std::shared_ptr<const opossum::Table> referenced_table,
                                   const opossum::ColumnID referenced_column_id,
                                   const std::shared_ptr<const opossum::PosList> pos)
    : _referenced_table(referenced_table), _referenced_column_id(referenced_column_id), _pos_list(pos) {
  Assert(_referenced_column_id < _referenced_table->column_count(),
         "Referenced column id does not exist in referenced table.");
  Assert(referenced_table != nullptr, "Referenced table cannot be null.");
  Assert(pos != nullptr, "List of positions cannot be null.");
}

const AllTypeVariant ReferenceSegment::operator[](const size_t offset) const {
  DebugAssert(offset < _pos_list->size(), "ReferenceSegment offset is out of bounds.");
  const RowID& row_id = (*_pos_list)[offset];

  DebugAssert(row_id.chunk_id < _referenced_table->chunk_count(), "Target chunk id is out of bounds.");
  const Chunk& referenced_chunk = _referenced_table->get_chunk(row_id.chunk_id);

  DebugAssert(row_id.chunk_offset < referenced_chunk.size(), "Target chunk offset is out of bounds.");
  const auto& segment = referenced_chunk.get_segment(_referenced_column_id);
  return (*segment)[row_id.chunk_offset];
}

size_t ReferenceSegment::size() const { return _pos_list->size(); }

const std::shared_ptr<const PosList> ReferenceSegment::pos_list() const { return _pos_list; }

const std::shared_ptr<const Table> ReferenceSegment::referenced_table() const { return _referenced_table; }

ColumnID ReferenceSegment::referenced_column_id() const { return _referenced_column_id; }

}  // namespace opossum
