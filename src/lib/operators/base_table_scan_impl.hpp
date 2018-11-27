#pragma once

#include <memory>

#include "abstract_operator.hpp"
#include "all_type_variant.hpp"
#include "types.hpp"

#include "storage/table.hpp"

namespace opossum {

/**
 * abstract class for TableScanImpl, this class serves as superclass of the templated
 * TableScanImpl class. Since a TableScanImpl object shall be member of class TableScan,
 * but the templated type is just known while runtime, a not templated class is required.
 */
class BaseTableScanImpl : private Noncopyable {
 public:
  BaseTableScanImpl() = default;

  virtual ~BaseTableScanImpl() = default;

  virtual std::shared_ptr<const Table> execute() const = 0;
};

}  // namespace opossum
