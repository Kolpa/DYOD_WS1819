#pragma once

#include <memory>

#include "abstract_operator.hpp"
#include "all_type_variant.hpp"
#include "types.hpp"

namespace opossum {

class Table;

class TableScan : public AbstractOperator {
 public:
  explicit TableScan(const std::shared_ptr<const AbstractOperator> in, ColumnID column_id, const ScanType scan_type,
                     const AllTypeVariant search_value);

  ColumnID column_id() const;
  ScanType scan_type() const;
  AllTypeVariant search_value() const;

 protected:
  const ColumnID _column_id;
  const ScanType _scan_type;
  const AllTypeVariant _search_value;

  std::shared_ptr<const Table> _on_execute() override;
};

}  // namespace opossum
