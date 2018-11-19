#pragma once

#include <algorithm>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "base_attribute_vector.hpp"
#include "fitted_attribute_vector.hpp"
#include "value_segment.hpp"

#include "all_type_variant.hpp"
#include "type_cast.hpp"
#include "types.hpp"

namespace opossum {

class BaseAttributeVector;

class BaseSegment;

// Even though ValueIDs do not have to use the full width of ValueID (uint32_t), this will also work for smaller ValueID
// types (uint8_t, uint16_t) since after a down-cast INVALID_VALUE_ID will look like their numeric_limit::max()
constexpr ValueID INVALID_VALUE_ID{std::numeric_limits<ValueID::base_type>::max()};

#define _BOUND(BOUND_TYPE)                                                                    \
  const auto occurrence_iter = BOUND_TYPE(_dictionary->cbegin(), _dictionary->cend(), value); \
  if (occurrence_iter != _dictionary->cend()) {                                               \
    return static_cast<ValueID>(std::distance(_dictionary->cbegin(), occurrence_iter));       \
  }                                                                                           \
  return INVALID_VALUE_ID;

// Dictionary is a specific segment type that stores all its values in a vector
template <typename T>
class DictionarySegment : public BaseSegment {
 public:
  /**
   * Creates a Dictionary segment from a given value segment.
   */
  explicit DictionarySegment(const std::shared_ptr<BaseSegment>& base_segment) {
    // We imply that BaseSegment will always be a value segment.
    // If not the code should fail in the next line.
    DebugAssert(std::dynamic_pointer_cast<ValueSegment<T>>(base_segment) != nullptr,
                "base_segment must be of type ValueSegment");
    const auto value_segment = std::static_pointer_cast<ValueSegment<T>>(base_segment);
    _initialize_dictionary(value_segment);
    _initialize_attribute_vector(value_segment);
  }

  // SEMINAR INFORMATION: Since most of these methods depend on the template parameter, you will have to implement
  // the DictionarySegment in this file. Replace the method signatures with actual implementations.

  // return the value at a certain position. If you want to write efficient operators, back off!
  const AllTypeVariant operator[](const size_t i) const override { return get(i); }

  // return the value at a certain position.
  const T get(const size_t i) const { return _dictionary->at(_attribute_vector->get(i)); }

  // dictionary segments are immutable
  void append(const AllTypeVariant&) override {
    throw std::runtime_error("Appending value to dictionary segment failed: Dictionary segments are immutable.");
  }

  // returns an underlying dictionary
  std::shared_ptr<const std::vector<T>> dictionary() const { return _dictionary; }

  // returns an underlying data structure
  std::shared_ptr<const BaseAttributeVector> attribute_vector() const { return _attribute_vector; }

  // return the value represented by a given ValueID
  const T& value_by_value_id(ValueID value_id) const { return _dictionary->at(value_id); }

  // returns the first value ID that refers to a value >= the search value
  // returns INVALID_VALUE_ID if all values are smaller than the search value
  ValueID lower_bound(T value) const {_BOUND(std::lower_bound)}

  // same as lower_bound(T), but accepts an AllTypeVariant
  ValueID lower_bound(const AllTypeVariant& value) const { return lower_bound(type_cast<T>(value)); }

  // returns the first value ID that refers to a value > the search value
  // returns INVALID_VALUE_ID if all values are smaller than or equal to the search value
  ValueID upper_bound(T value) const {_BOUND(std::upper_bound)}

  // same as upper_bound(T), but accepts an AllTypeVariant
  ValueID upper_bound(const AllTypeVariant& value) const { return upper_bound(type_cast<T>(value)); }

  // return the number of unique_values (dictionary entries)
  size_t unique_values_count() const { return _dictionary->size(); }

  // return the number of entries
  size_t size() const override { return _attribute_vector->size(); }

 protected:
  std::shared_ptr<std::vector<T>> _dictionary;
  std::shared_ptr<BaseAttributeVector> _attribute_vector;

  // initializes the dictionary using the value segment
  void _initialize_dictionary(const std::shared_ptr<ValueSegment<T>>& value_segment) {
    // copy all values to dictionary
    _dictionary = std::make_shared<std::vector<T>>(value_segment->values());
    std::sort(_dictionary->begin(), _dictionary->end());
    const auto begin_erase_iter = std::unique(_dictionary->begin(), _dictionary->end());
    _dictionary->erase(begin_erase_iter, _dictionary->cend());
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
      throw std::runtime_error("Not implemented attribute vector size.");
    }
  }

  // Initializes the attribute vector where U is the type of the value IDs
  template <typename U>
  void _initialize_attribute_vector(const std::shared_ptr<ValueSegment<T>>& value_segment) {
    std::vector<U> value_ids;
    value_ids.reserve(value_segment->size());

    std::transform(value_segment->values().cbegin(), value_segment->values().cend(), std::back_inserter(value_ids),
                   [&](const T& value) { return lower_bound(value); });

    _attribute_vector = std::make_shared<FittedAttributeVector<U>>(std::move(value_ids));
    DebugAssert(value_ids.empty(), "value_ids should be moved");
  }
};

}  // namespace opossum
