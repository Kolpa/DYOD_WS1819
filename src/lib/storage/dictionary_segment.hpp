#pragma once

#include <algorithm>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base_attribute_vector.hpp"
#include "fitted_attribute_vector.hpp"
#include "value_segment.hpp"

#include "all_type_variant.hpp"
#include "types.hpp"

namespace opossum {

class BaseAttributeVector;

class BaseSegment;

// Even though ValueIDs do not have to use the full width of ValueID (uint32_t), this will also work for smaller ValueID
// types (uint8_t, uint16_t) since after a down-cast INVALID_VALUE_ID will look like their numeric_limit::max()
constexpr ValueID INVALID_VALUE_ID{std::numeric_limits<ValueID::base_type>::max()};

// Dictionary is a specific segment type that stores all its values in a vector
template<typename T>
class DictionarySegment : public BaseSegment {
 public:
  /**
   * Creates a Dictionary segment from a given value segment.
   */
  explicit DictionarySegment(const std::shared_ptr<BaseSegment>& base_segment) {

    const auto value_segment = std::static_pointer_cast<ValueSegment<T>>(base_segment);
    _initialize_dictionary(value_segment);
    _initialize_attribute_vector(value_segment);
  }

  // SEMINAR INFORMATION: Since most of these methods depend on the template parameter, you will have to implement
  // the DictionarySegment in this file. Replace the method signatures with actual implementations.

  // return the value at a certain position. If you want to write efficient operators, back off!
  const AllTypeVariant operator[](const size_t i) const {
    return get(i);
  }

  // return the value at a certain position.
  const T get(const size_t i) const {
    return _dictionary->at(_attribute_vector->get(i));
  }

  // dictionary segments are immutable
  void append(const AllTypeVariant&) {
    throw std::runtime_error("Dictionary segments are immutable");
  }

  // returns an underlying dictionary
  std::shared_ptr<const std::vector<T>> dictionary() const {
    return _dictionary;
  }

  // returns an underlying data structure
  std::shared_ptr<const BaseAttributeVector> attribute_vector() const {
    return _attribute_vector;
  }

  // return the value represented by a given ValueID
  const T& value_by_value_id(ValueID value_id) const {
    return _dictionary[value_id];
  }

  // returns the first value ID that refers to a value >= the search value
  // returns INVALID_VALUE_ID if all values are smaller than the search value
  ValueID lower_bound(T value) const {
    return _index_of(value);
  }

  // same as lower_bound(T), but accepts an AllTypeVariant
  ValueID lower_bound(const AllTypeVariant& value) const {
    // Todo: Cast required?
    return lower_bound(static_cast<T>(value));
  }

  // returns the first value ID that refers to a value > the search value
  // returns INVALID_VALUE_ID if all values are smaller than or equal to the search value
  ValueID upper_bound(T value) const {
    const auto it = std::upper_bound(_dictionary->cbegin(), _dictionary->cend(), value);
    if (it == _dictionary->cend()) {
      return INVALID_VALUE_ID;
    }

    return ValueID {static_cast<uint32_t>(std::distance(_dictionary->cbegin(), it))};
  }

  // same as upper_bound(T), but accepts an AllTypeVariant
  ValueID upper_bound(const AllTypeVariant& value) const {
    // Todo: Cast required?
    return upper_bound(static_cast<T>(value));
  }

  // return the number of unique_values (dictionary entries)
  size_t unique_values_count() const {
    return _dictionary->size();
  }

  // return the number of entries
  size_t size() const {
    return _attribute_vector->size();
  }

 protected:
  std::shared_ptr<std::vector<T>> _dictionary;
  std::shared_ptr<BaseAttributeVector> _attribute_vector;

  // initializes the dictionary using the value segment
  void _initialize_dictionary(const std::shared_ptr<ValueSegment<T>>& value_segment) {
    // copy all values to dictionary
    _dictionary = std::make_shared<std::vector<T>>(std::move(value_segment->values()));
    std::sort(_dictionary->begin(), _dictionary->end());
    auto last = std::unique(_dictionary->begin(), _dictionary->end());
    _dictionary->erase(last, _dictionary->end());
    // we want to enforce (hopefully) that the dictionary requires less memory than the attribute vector
    _dictionary->shrink_to_fit();
  }

  // initializes the attribute vector using the current dictionary.
  // must be called after initialize_dictionary
  void _initialize_attribute_vector(const std::shared_ptr<ValueSegment<T>>& value_segment) {

    if (unique_values_count() <= std::numeric_limits<uint8_t>::max()) {
      _initialize_attribute_vector<uint8_t>(value_segment);
    } else if (unique_values_count() <= std::numeric_limits<uint16_t>::max()) {
      _initialize_attribute_vector<uint16_t>(value_segment);
    } else if (unique_values_count() <= std::numeric_limits<uint32_t>::max()) {
      _initialize_attribute_vector<uint32_t>(value_segment);
    } else {
      // TODO: We don't actually need this beacuse we can only have up to std::numeric_limits<uint32_t>::max())
      // values in a chunk;
      _initialize_attribute_vector<uint64_t>(value_segment);
    }

  }

  template <typename U>
  void _initialize_attribute_vector(const std::shared_ptr<ValueSegment<T>>& value_segment) {
    std::vector<U> value_ids;
    value_ids.reserve(value_segment->size());


    std::transform(value_segment->values().cbegin(), value_segment->values().cend(), std::back_inserter(value_ids),
        [&](const T& value) { return  _index_of(value); });


    _attribute_vector = std::make_shared<FittedAttributeVector<U>>(std::move(value_ids));
  }

  ValueID _index_of(const T& value) const
  {
    // TODO: Test if correct.
    const auto it = std::lower_bound(_dictionary->cbegin(), _dictionary->cend(), value);
    if (it == _dictionary->cend()) {
      return INVALID_VALUE_ID;
    }

    return ValueID {static_cast<uint32_t>(std::distance(_dictionary->cbegin(), it))};
  }


};

}  // namespace opossum
