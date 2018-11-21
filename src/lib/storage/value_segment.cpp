#include "value_segment.hpp"

#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "type_cast.hpp"
#include "utils/assert.hpp"
#include "utils/performance_warning.hpp"

namespace opossum {

template <typename T>
const AllTypeVariant ValueSegment<T>::operator[](const size_t offset) const {
  PerformanceWarning("operator[] used");
  DebugAssert(offset < _values.size(), "Offset is ouf of bounds.");
  return _values[offset];
}

template <typename T>
void ValueSegment<T>::append(const AllTypeVariant& val) {
  const auto value = type_cast<T>(val);
  _values.push_back(value);
}

template <typename T>
size_t ValueSegment<T>::size() const {
  return _values.size();
}

template <typename T>
const std::vector<T>& ValueSegment<T>::values() const {
  return _values;
}

EXPLICITLY_INSTANTIATE_DATA_TYPES(ValueSegment);

}  // namespace opossum
