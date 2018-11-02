#pragma once

#include <vector>

#include "base_attribute_vector.hpp"

#include "types.hpp"

namespace opossum {

template <typename T>
class FittedAttributeVector : public BaseAttributeVector {
 public:

  explicit FittedAttributeVector(std::vector<T>&& values)  : _values{values} {}

  // we need to explicitly set the move constructor to default when
  // we overwrite the copy constructor
  FittedAttributeVector(FittedAttributeVector&&) = default;
  FittedAttributeVector& operator=(FittedAttributeVector&&) = default;

  // returns the value id at a given position
  ValueID get(const size_t i) const {
    return ValueID{static_cast<uint32_t>(_values[i])};
  }

  // sets the value id at a given position
  void set(const size_t i, const ValueID value_id) {
    _values[i] = static_cast<T>(value_id);
  }

  // returns the number of values
  size_t size() const {
    return _values.size();
  }

  // returns the width of biggest value id in bytes
  AttributeVectorWidth width() const {
    return AttributeVectorWidth{sizeof(T)};
  }

 protected:
  std::vector<T> _values;
};
}  // namespace opossum
