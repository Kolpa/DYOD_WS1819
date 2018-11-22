#pragma once

#include <storage/reference_segment.hpp>

#include <memory>
#include <utility>

#include "base_table_scan_impl.hpp"
#include "storage/chunk.hpp"
#include "storage/table.hpp"
#include "type_cast.hpp"

namespace opossum {

// todo(anyone) overall: improve structure & performance

template <typename T>
class TableScanImpl : public BaseTableScanImpl {
 public:
  TableScanImpl(const std::shared_ptr<const AbstractOperator> in, ColumnID column_id,
                const ScanType comparison_operator, const AllTypeVariant comparison_value)
      : BaseTableScanImpl(in, column_id, comparison_operator, comparison_value),
        _comparison_value(type_cast<T>(comparison_value)) {}

 protected:
  // member variables
  const T _comparison_value;
  // member methods
  std::shared_ptr<const Table> _on_execute() override {
    std::shared_ptr<const Table> table_in = _input_left->get_output();
    std::shared_ptr<Table> table_out = std::make_shared<Table>();

    // create the table definition (name, type) for the output table
    // use the same definition as in the input table
    for (ColumnID column_index{0}; column_index < table_in->column_count(); ++column_index) {
      table_out->add_column_definition(table_in->column_name(column_index), table_in->column_type(column_index));
    }
    // create for each chunk of the input table a chunk for the output table that just contains
    // reference segments, these segments just contain references to values that are accepted by
    // comparison operation

    for (ChunkID chunk_index{0}; chunk_index < table_in->chunk_count(); ++chunk_index) {
      const auto& segment_to_scan = table_in->get_chunk(chunk_index).get_segment(_column_id);
      const auto& pos_list = _get_positions_of_accepted_values(chunk_index, segment_to_scan);
      if (!pos_list->empty()) {
        Chunk chunk_to_add{};
        for (ColumnID column_index{0}; column_index < table_in->column_count(); ++column_index) {
          chunk_to_add.add_segment(std::make_shared<ReferenceSegment>(table_in, column_index, pos_list));
        }
        table_out->emplace_chunk(std::move(chunk_to_add));
      }
    }
    return table_out;
  }

 private:
  // TODO(anyone) test this
  bool _accepted_by_comparison(const T& value) const {
    switch (_comparison_operator) {
      case ScanType::OpEquals:
        return value == _comparison_value;
      case ScanType::OpGreaterThan:
        return value > _comparison_value;
      case ScanType::OpGreaterThanEquals:
        return value >= _comparison_value;
      case ScanType::OpLessThan:
        return value < _comparison_value;
      case ScanType::OpLessThanEquals:
        return value <= _comparison_value;
      case ScanType::OpNotEquals:
        return value != _comparison_value;
      default:
        throw std::runtime_error("comparison operator is not supported.");
    }
  }

  const std::shared_ptr<PosList> _get_positions_of_accepted_values(ChunkID& chunkID,
                                                                   std::shared_ptr<BaseSegment> segment) {
    std::shared_ptr<PosList> pos_list = std::make_shared<PosList>();
    // TODO(anyone)
    // - check which segment type the passed 'segment' is.
    // - check for each value if it qualifies regarding the defined operation
    //      use _accepted_by_comparison? test the method?
    // - if a value qualifies, add the position of that value to a PosList
    //     remember: PosList is a vector of RowIDs
    //     remember: a RowID is  a composition of ChunkID and ChunkOffset
    //
    return pos_list;
  }
};

}  // namespace opossum
