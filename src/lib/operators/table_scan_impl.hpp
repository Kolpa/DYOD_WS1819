#pragma once

#include <memory>
#include <utility>

#include "base_table_scan_impl.hpp"
#include "type_cast.hpp"
#include "types.hpp"

#include "utils/assert.hpp"
#include "storage/chunk.hpp"
#include <storage/reference_segment.hpp>
#include "storage/table.hpp"

namespace opossum {

template<typename T>
class TableScanImpl : public BaseTableScanImpl {
 public:
  TableScanImpl(const std::shared_ptr<const Table>& input_table,
                const ColumnID column_id, const ScanType scan_type,
                const AllTypeVariant search_value)
      : BaseTableScanImpl(), _input_table(input_table), _column_id(column_id),
        _scan_type(scan_type), _search_value(type_cast<T>(search_value)) {
    DebugAssert(input_table != nullptr, "Input table must be defined.");
  }

 protected:

  const std::shared_ptr<const Table> _input_table;
  const ColumnID _column_id;
  const ScanType _scan_type;
  const T _search_value;

  std::shared_ptr<const Table> execute() const override {
    auto output_table = std::make_shared<Table>();

    // initializes the schema of the output table.
    for (ColumnID column_index{0}; column_index < _input_table->column_count(); ++column_index) {
      output_table->add_column_definition(_input_table->column_name(column_index),
                                          _input_table->column_type(column_index));
    }

    // Iterate through all values of the input table.
    // All values, which fulfill the filter criterium, will be added to the output table;

    for (ChunkID chunk_id{0}; chunk_id < _input_table->chunk_count(); ++chunk_id) {
      const auto& segment_to_scan = _input_table->get_chunk(chunk_id).get_segment(_column_id);


      /*Jetzt iterieren wir eigentlich nur durch die segmente*/
      // Im Falle vom reference_segment holen wir das entsprechende Segment und iterieren dann, abhängig von der
      // segmentart dardurch.


      std::shared_ptr<PosList> pos_list;

      // Find out out the type of the segment by using dynamic pointer casts.
      const auto& value_segment = std::dynamic_pointer_cast<ValueSegment<T>>(segment_to_scan);
      if (value_segment != nullptr) {
        pos_list = _get_positions_of_accepted_values(chunk_id, value_segment);
      } else {
        const auto dictionary_segment = std::dynamic_pointer_cast<DictionarySegment<T>>(segment_to_scan);
        if (dictionary_segment != nullptr) {
          pos_list = _get_positions_of_accepted_values(chunk_id, dictionary_segment);
        } else {
          const auto reference_segment = std::dynamic_pointer_cast<ReferenceSegment>(segment_to_scan);
          if (reference_segment == nullptr) {
            throw std::runtime_error("Unsupported segment type.");
          }
          pos_list = _get_positions_of_accepted_values(chunk_id, reference_segment);
        }
      }

      if (!pos_list->empty()) {
        auto chunk_to_add = _create_chunk(pos_list);
        output_table->emplace_chunk(std::move(chunk_to_add));
      }
    }

    if (output_table->row_count() == 0) {
      output_table->emplace_chunk(std::move(_create_chunk(std::make_shared<PosList>())));
    }

    return output_table;
  }

  Chunk _create_chunk(const std::shared_ptr<PosList> pos_list) const {
    Chunk chunk;
    for (ColumnID column_id{0}; column_id < _input_table->column_count(); ++column_id) {
      chunk.add_segment(std::make_shared<ReferenceSegment>(_input_table, column_id, pos_list));
    }
    return chunk;
  }

  bool _accepted_by_comparison(const T& value) const {
    switch (_scan_type) {
      case ScanType::OpEquals:
        return value == _search_value;
      case ScanType::OpGreaterThan:
        return value > _search_value;
      case ScanType::OpGreaterThanEquals:
        return value >= _search_value;
      case ScanType::OpLessThan:
        return value < _search_value;
      case ScanType::OpLessThanEquals:
        return value <= _search_value;
      case ScanType::OpNotEquals:
        return value != _search_value;
      default:
        throw std::runtime_error("comparison operator is not supported.");
    }
  }

  std::shared_ptr<PosList> _get_positions_of_accepted_values(const ChunkID& chunkID,
                                                             const std::shared_ptr<ValueSegment<T>>& segment) const {
    std::shared_ptr<PosList> pos_list = std::make_shared<PosList>();
    pos_list->reserve(segment->size() / 2);

    auto offset = ChunkOffset{0};
    for (const auto& value : segment->values()) {
      if (_accepted_by_comparison(value)) {
        pos_list->emplace_back(RowID{chunkID, offset});
      }
      ++offset;
    }

    return pos_list;
  }

  std::shared_ptr<PosList> _get_positions_of_accepted_values(const ChunkID& chunkID,
                                                             const std::shared_ptr<DictionarySegment<T>>& segment) const {
    std::shared_ptr<PosList> pos_list = std::make_shared<PosList>();
    pos_list->reserve(segment->size() / 2);

    const auto& attribute_vector = segment->attribute_vector();

    for (size_t i = 0; i < attribute_vector->size(); ++i) {
      const auto& value = segment->value_by_value_id(attribute_vector->get(i));
      if (_accepted_by_comparison(value)) {
        pos_list->emplace_back(RowID{chunkID, ChunkOffset(i)});
      }
    }

    return pos_list;
  }

  std::shared_ptr<PosList> _get_positions_of_accepted_values(const ChunkID& chunkID,
                                                             const std::shared_ptr<ReferenceSegment>& segment) const {
    std::shared_ptr<PosList> pos_list = std::make_shared<PosList>();
    pos_list->reserve(segment->size() / 2);

    for (size_t i = 0; i < segment->size(); ++i) {
      const auto& value = type_cast<T>((*segment)[i]);
      if (_accepted_by_comparison(value)) {
        pos_list->push_back((*(segment->pos_list()))[i]);
      }
    }

    return pos_list;
  }
};

}  // namespace opossum
