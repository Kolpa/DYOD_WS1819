#include "dictionary_segment.hpp"

#include "value_segment.hpp"
#include "fitted_attribute_vector.hpp"
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace opossum {

// Creates a Dictionary segment from a given value segment.
template <class T>
DictionarySegment<T>::DictionarySegment(const std::shared_ptr<BaseSegment>& base_segment) {
  const auto value_segment = std::dynamic_pointer_cast<ValueSegment<T>>(base_segment);
  _init_dictionary(value_segment);
  _init_attribute_vector(value_segment);
}

// SEMINAR INFORMATION: Since most of these methods depend on the template parameter, you will have to implement
// the DictionarySegment in this file. Replace the method signatures with actual implementations.

// return the value at a certain position. If you want to write efficient operators, back off!
template <typename T>
const AllTypeVariant DictionarySegment<T>::operator[](const size_t i) const {
  throw std::runtime_error("Implementation missing.");
}

// return the value at a certain position.
template <typename T>
const T DictionarySegment<T>::get(const size_t i) const {
  throw std::runtime_error("Implementation missing.");
}

// dictionary segments are immutable
template <typename T>
void DictionarySegment<T>::append(const AllTypeVariant&) {
  throw std::runtime_error("Implementation missing.");
}

// returns an underlying dictionary
template <typename T>
std::shared_ptr<const std::vector<T>> DictionarySegment<T>::dictionary() const {
  throw std::runtime_error("Implementation missing.");
}

// returns an underlying data structure
template <typename T>
std::shared_ptr<const BaseAttributeVector> DictionarySegment<T>::attribute_vector() const {
  throw std::runtime_error("Implementation missing.");
}

// return the value represented by a given ValueID
template <typename T>
const T& DictionarySegment<T>::value_by_value_id(ValueID value_id) const {
  throw std::runtime_error("Implementation missing.");
}

// returns the first value ID that refers to a value >= the search value
// returns INVALID_VALUE_ID if all values are smaller than the search value
template <typename T>
ValueID DictionarySegment<T>::lower_bound(T value) const {
  throw std::runtime_error("Implementation missing.");
}

// same as lower_bound(T), but accepts an AllTypeVariant
template <typename T>
ValueID DictionarySegment<T>::lower_bound(const AllTypeVariant& value) const {
  throw std::runtime_error("Implementation missing.");
}

// returns the first value ID that refers to a value > the search value
// returns INVALID_VALUE_ID if all values are smaller than or equal to the search value
template <typename T>
ValueID DictionarySegment<T>::upper_bound(T value) const {
  throw std::runtime_error("Implementation missing.");
}

// same as upper_bound(T), but accepts an AllTypeVariant
template <typename T>
ValueID DictionarySegment<T>::upper_bound(const AllTypeVariant& value) const {
  throw std::runtime_error("Implementation missing.");
}

// return the number of unique_values (dictionary entries)
template <typename T>
size_t DictionarySegment<T>::unique_values_count() const {
  throw std::runtime_error("Implementation missing.");
}

// return the number of entries
template <typename T>
size_t DictionarySegment<T>::size() const {
  throw std::runtime_error("Implementation missing.");
}

template <typename T>
void DictionarySegment<T>::_init_dictionary(const std::shared_ptr<ValueSegment<T>>& value_segment) {
  for(const auto& seg_value: value_segment->values()){
    auto dict_iter = _dictionary->cbegin();
    do{
      // insert the segment value if the end of dictionary is reached or
      // if the segment value is less than the current iterated dict value
      if(dict_iter == _dictionary->cend() || seg_value < *dict_iter){
        dict_iter = _dictionary->insert(dict_iter, seg_value);
      }
      ++dict_iter;
    }while((dict_iter != _dictionary->cend()));
  }
}

template <typename T>
void DictionarySegment<T>::_init_attribute_vector(const std::shared_ptr<ValueSegment<T>>& value_segment) {
  if(value_segment->size() <= std::numeric_limits<uint8_t>::max()){
    _attribute_vector = std::make_shared<FittedAttributeVector<uint8_t >>();
  }else if(value_segment->size() <= std::numeric_limits<uint16_t>::max()){
    _attribute_vector = std::make_shared<FittedAttributeVector<uint16_t >>();
  }else{
    _attribute_vector = std::make_shared<FittedAttributeVector<uint32_t >>();
  }

  ChunkOffset att_vec_index = 0;
  for(const auto& value : value_segment->values()){
    const auto occurence_iter = std::find(_dictionary->cbegin(), _dictionary->cend(), value);
    DebugAssert(occurence_iter != _dictionary->cend(), "Value is not available in the dictionary.");
    uint32_t dict_pos = std::distance(_dictionary->cbegin(), occurence_iter);
    _attribute_vector->set(att_vec_index, ValueID{dict_pos});
    ++att_vec_index;
  }
}

   EXPLICITLY_INSTANTIATE_DATA_TYPES(DictionarySegment);


}  // namespace opossum