#pragma once

#include <utility>
#include <vector>

#include "base_attribute_vector.hpp"

#include "../utils/assert.hpp"
#include "types.hpp"

namespace opossum {

template <typename T>
class FittedAttributeVector : public BaseAttributeVector {
 public:
  explicit FittedAttributeVector(std::vector<T>&& values) : _values{std::move(values)} {
    bool valid_type = std::is_same_v<T, uint8_t> || std::is_same_v<T, uint16_t> || std::is_same_v<T, uint32_t>;
    Assert(valid_type, "Template type has to be uint8_t, uint16_t or uint32_t");
  }

  // we need to explicitly set the move constructor to default when
  // we overwrite the copy constructor
  FittedAttributeVector(FittedAttributeVector&&) = default;
  FittedAttributeVector& operator=(FittedAttributeVector&&) = default;

  // returns the value id at a given position
  ValueID get(const size_t offset) const {
    DebugAssert(offset < _values.size(), "Offset is out of bounds.");
    return static_cast<ValueID>(_values[offset]);
  }

  // sets the value id at a given position
  void set(const size_t offset, const ValueID value_id) {
    DebugAssert(offset < _values.size(), "Offset is out of bounds.");
    _values[offset] = static_cast<T>(value_id);
  }

  // returns the number of values
  size_t size() const { return _values.size(); }

  // returns the width of biggest value id in bytes
  AttributeVectorWidth width() const { return AttributeVectorWidth{sizeof(T)}; }

 protected:
  std::vector<T> _values;
};
}  // namespace opossum
