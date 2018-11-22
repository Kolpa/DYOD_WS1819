#include "table_scan.hpp"

#include <memory>

#include "resolve_type.hpp"
#include "storage/table.hpp"
#include "table_scan_impl.hpp"

namespace opossum {

TableScan::TableScan(const std::shared_ptr<const AbstractOperator> in, ColumnID column_id, const ScanType scan_type,
                     const AllTypeVariant search_value)
    : AbstractOperator(in) {
  // get the type if the corresponding column of the input table
  const auto& data_type = _input_left->get_output()->column_type(column_id);
  _table_scan_impl = opossum::make_unique_by_data_type<BaseTableScanImpl, TableScanImpl>(data_type, in, column_id,
                                                                                         scan_type, search_value);
}

ColumnID TableScan::column_id() const { return _table_scan_impl->column_id(); }

ScanType TableScan::scan_type() const { return _table_scan_impl->comparison_operator(); }

const AllTypeVariant& TableScan::search_value() const { return _table_scan_impl->comparison_value(); }

std::shared_ptr<const Table> TableScan::_on_execute() {
  _table_scan_impl->execute();
  return _table_scan_impl->get_output();
}

}  // namespace opossum
