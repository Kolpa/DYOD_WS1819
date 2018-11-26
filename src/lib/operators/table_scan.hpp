#pragma once

#include "abstract_operator.hpp"
#include "all_type_variant.hpp"
#include "storage/table.hpp"
#include "types.hpp"

// TODO(anyone)
/* - single chunk with referenceSegments that point to the found values
 * - do not produce empty chunks, unless the result is empty, in which case we produce a single empty chunk
 * - do not use operator[] on columns due to the virtual method calls involved and the use of AllTypeVariant
 * - therefore get the ValueSegments value vector or the DictionarySegments' att. vect. and the dict.
 *      when you want to scna the values.
 * - TableScan operator is not templated
 * - checking columns type: std::dynamic_pointer_cast<DictionarySegment>(b)
 *      if b can be casted to DictionarySegment, the pointer cast returns such a pointer,
 *      else it returns nullptr
 * - comparision: we will not deal with varying data types. BUT if the types do not match,
 *      the implementation should notice this and throw an exception.
 * - comparison is trivial if: T value_to_be_compared_to and the value vector const std::vector<T>& are available.
 * - ...
 */

namespace opossum {

class BaseTableScanImpl;
class Table;

class TableScan : public AbstractOperator {
 public:
  TableScan(const std::shared_ptr<const AbstractOperator> in, ColumnID column_id, const ScanType scan_type,
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
