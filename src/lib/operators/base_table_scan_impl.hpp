#pragma once

#include <memory>

#include "abstract_operator.hpp"
#include "all_type_variant.hpp"

namespace opossum {

/**
 * abstract class for TableScanImpl, this class serves as superclass of the templated
 * TableScanImpl class. Since an TableScanImpl object shall be member of class TableScan,
 * but the templated type is just known while runtime, a not templated class is required.
 */
class BaseTableScanImpl : public AbstractOperator {
 public:
  BaseTableScanImpl(const std::shared_ptr<const AbstractOperator> in, ColumnID column_id,
                    const ScanType comparison_operator, const AllTypeVariant comparison_value);

  virtual ~BaseTableScanImpl() = default;

  BaseTableScanImpl(BaseTableScanImpl&&) = default;

  BaseTableScanImpl& operator=(BaseTableScanImpl&&) = default;

  ColumnID column_id() const;

  ScanType comparison_operator() const;

  const AllTypeVariant& comparison_value();

 protected:
  const ColumnID _column_id;
  const ScanType _comparison_operator;
  // right-hand side operand of the comparison operation/reference value/comparison value
  const AllTypeVariant _comparison_value;
};

}  // namespace opossum
