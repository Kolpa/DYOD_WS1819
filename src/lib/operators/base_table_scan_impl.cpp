#include "base_table_scan_impl.hpp"
#include <memory>

namespace opossum {

BaseTableScanImpl::BaseTableScanImpl(const std::shared_ptr<const opossum::AbstractOperator> in,
                                     opossum::ColumnID column_id, const opossum::ScanType comparison_operator,
                                     const opossum::AllTypeVariant comparison_value)
    : AbstractOperator(in),
      _column_id(column_id),
      _comparison_operator(comparison_operator),
      _comparison_value(comparison_value) {}

ColumnID BaseTableScanImpl::column_id() const { return _column_id; }

ScanType BaseTableScanImpl::comparison_operator() const { return _comparison_operator; }

const AllTypeVariant& BaseTableScanImpl::comparison_value() { return _comparison_value; }

}  // namespace opossum
