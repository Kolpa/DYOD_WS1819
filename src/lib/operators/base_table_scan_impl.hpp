#pragma once

#include <memory>

#include "types.hpp"

namespace opossum {

class Table;

/**
 * Abstract, untemplated base class for TableScanImpl.
 */
class BaseTableScanImpl : private Noncopyable {
 public:
  virtual ~BaseTableScanImpl() = default;

  virtual std::shared_ptr<const Table> execute() const = 0;
};

}  // namespace opossum
