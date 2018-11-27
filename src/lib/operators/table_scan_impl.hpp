#pragma once

#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "base_table_scan_impl.hpp"
#include "segment_scanner.hpp"
#include "type_cast.hpp"
#include "types.hpp"

#include "storage/chunk.hpp"
#include "storage/reference_segment.hpp"
#include "storage/table.hpp"
#include "utils/assert.hpp"

namespace opossum {

template <typename T>
class TableScanImpl : public BaseTableScanImpl {
 public:
  TableScanImpl(const std::shared_ptr<const Table>& input_table, const ColumnID column_id, const ScanType scan_type,
                const AllTypeVariant search_value)
      : BaseTableScanImpl(),
        _input_table(input_table),
        _column_id(column_id),
        _scan_type(scan_type),
        _search_value(type_cast<T>(search_value)) {
    DebugAssert(input_table != nullptr, "Input table must be defined.");
  }

 protected:
  const std::shared_ptr<const Table>& _input_table;
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

    const auto scanner = AbstractScanner<T>::from_scan_type(_scan_type);

    // Iterate through all values of the input table.
    // All values, which fulfill the filter criterion, will be added to the output table;

    for (ChunkID chunk_id{0}; chunk_id < _input_table->chunk_count(); ++chunk_id) {
      const auto segment_to_scan = _input_table->get_chunk(chunk_id).get_segment(_column_id);
      const auto pos_list = std::make_shared<const PosList>(scanner->scan(chunk_id, segment_to_scan, _search_value));

      if (!pos_list->empty()) {
        Chunk chunk_to_add;
        if (const auto& ref_seg = std::dynamic_pointer_cast<ReferenceSegment>(segment_to_scan)) {
          chunk_to_add = _create_chunk(pos_list, ref_seg->referenced_table());
        }
        else {
          chunk_to_add = _create_chunk(pos_list, _input_table);
        }

        output_table->emplace_chunk(std::move(chunk_to_add));
      }
    }

    if (output_table->row_count() == 0) {
      output_table->emplace_chunk(std::move(_create_chunk(std::make_shared<PosList>(), _input_table)));
    }

    return output_table;
  }

  Chunk _create_chunk(const std::shared_ptr<const PosList>& pos_list, const std::shared_ptr<const Table>& table) const {
    DebugAssert(table != nullptr, "Input table is not initialized.");
    Chunk chunk;
    for (ColumnID column_id{0}; column_id < table->column_count(); ++column_id) {
      chunk.add_segment(std::make_shared<ReferenceSegment>(table, column_id, pos_list));
    }
    return chunk;
  }
};

}  // namespace opossum
