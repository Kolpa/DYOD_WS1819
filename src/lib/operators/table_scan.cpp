#include "table_scan.hpp"

#include <memory>

#include "base_table_scan_impl.hpp"
#include "resolve_type.hpp"
#include "storage/table.hpp"
#include "table_scan_impl.hpp"
#include "utils/assert.hpp"

namespace opossum {

TableScan::TableScan(const std::shared_ptr<const AbstractOperator> in, ColumnID column_id, const ScanType scan_type,
                     const AllTypeVariant search_value)
    : AbstractOperator(in), _column_id(column_id), _scan_type(scan_type), _search_value(search_value) {
  Assert(in != nullptr, "Input operator must be defined.");
}

ColumnID TableScan::column_id() const { return _column_id; }

ScanType TableScan::scan_type() const { return _scan_type; }

AllTypeVariant TableScan::search_value() const { return _search_value; }

std::shared_ptr<const Table> TableScan::_on_execute() {
  const auto input_table = _input_table_left();
  Assert(input_table != nullptr, "Input table must be defined.");

  const auto& data_type = input_table->column_type(column_id());
  auto table_scan = opossum::make_unique_by_data_type<BaseTableScanImpl, TableScanImpl>(
      data_type, input_table, column_id(), scan_type(), search_value());
  return table_scan->execute();
}

}  // namespace opossum
