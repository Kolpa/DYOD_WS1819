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
  AllTypeVariant var = _values[offset];
  return var;
}

template <typename T>
void ValueSegment<T>::append(const AllTypeVariant& val) {
  T t = type_cast<T>(val);
  _values.push_back(t);
}

template <typename T>
size_t ValueSegment<T>::size() const {
  return _values.size();
}

EXPLICITLY_INSTANTIATE_DATA_TYPES(ValueSegment);

}  // namespace opossum
