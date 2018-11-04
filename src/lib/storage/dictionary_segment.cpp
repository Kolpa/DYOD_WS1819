#include "dictionary_segment.hpp"

#include "value_segment.hpp"
#include "fitted_attribute_vector.hpp"

#include "type_cast.hpp"

namespace opossum {

// Creates a Dictionary segment from a given value segment.
    template<class T>
    DictionarySegment<T>::DictionarySegment(const std::shared_ptr<BaseSegment> &base_segment) {
        const auto value_segment = std::dynamic_pointer_cast<ValueSegment<T>>(base_segment);
        _init_dictionary(value_segment);
        _init_attribute_vector(value_segment);
    }

// SEMINAR INFORMATION: Since most of these methods depend on the template parameter, you will have to implement
// the DictionarySegment in this file. Replace the method signatures with actual implementations.

// return the value at a certain position. If you want to write efficient operators, back off!
    template<typename T>
    const AllTypeVariant DictionarySegment<T>::operator[](const size_t i) const {
        return this->get(i);
    }

// return the value at a certain position.
    template<typename T>
    const T DictionarySegment<T>::get(const size_t i) const {
        const ValueID value_id = _attribute_vector->get(i);
        return _dictionary->at(value_id);
    }

// dictionary segments are immutable
    template<typename T>
    void DictionarySegment<T>::append(const AllTypeVariant &) {
        throw std::runtime_error("Appending value to dictionary segment failed: Dictionary segments are immutable.");
    }

// returns an underlying dictionary
    template<typename T>
    std::shared_ptr<const std::vector<T>> DictionarySegment<T>::dictionary() const {
        return _dictionary;
    }

// returns an underlying data structure
    template<typename T>
    std::shared_ptr<const BaseAttributeVector> DictionarySegment<T>::attribute_vector() const {
        return _attribute_vector;
    }

// return the value represented by a given ValueID
    template<typename T>
    const T &DictionarySegment<T>::value_by_value_id(ValueID value_id) const {
        return _dictionary->at(value_id);
    }

// returns the first value ID that refers to a value >= the search value
// returns INVALID_VALUE_ID if all values are smaller than the search value
    template<typename T>
    ValueID DictionarySegment<T>::lower_bound(T value) const {
        const auto occurrence_iter = std::lower_bound(_dictionary->cbegin(), _dictionary->cend(), value);
        if (occurrence_iter != _dictionary->cend()) {
            return ValueID{static_cast<ValueID>(std::distance(_dictionary->cbegin(), occurrence_iter))};
        }
        return INVALID_VALUE_ID;
    }

// same as lower_bound(T), but accepts an AllTypeVariant
    template<typename T>
    ValueID DictionarySegment<T>::lower_bound(const AllTypeVariant &value) const {
        return lower_bound(type_cast<T>(value));
    }

// returns the first value ID that refers to a value > the search value
// returns INVALID_VALUE_ID if all values are smaller than or equal to the search value
    template<typename T>
    ValueID DictionarySegment<T>::upper_bound(T value) const {
        const auto occurrence_iter = std::upper_bound(_dictionary->cbegin(), _dictionary->cend(), value);
        if (occurrence_iter != _dictionary->cend()) {
            return ValueID{static_cast<ValueID>(std::distance(_dictionary->cbegin(), occurrence_iter))};
        }
        return INVALID_VALUE_ID;
    }

// same as upper_bound(T), but accepts an AllTypeVariant
    template<typename T>
    ValueID DictionarySegment<T>::upper_bound(const AllTypeVariant &value) const {
        return upper_bound(type_cast<T>(value));
    }

// return the number of unique_values (dictionary entries)
    template<typename T>
    size_t DictionarySegment<T>::unique_values_count() const {
        return _dictionary->size();
    }

// return the number of entries
    template<typename T>
    size_t DictionarySegment<T>::size() const {
        return _attribute_vector->size();
    }

    template<typename T>
    void DictionarySegment<T>::_init_dictionary(const std::shared_ptr<ValueSegment<T>> &value_segment) {
        _dictionary = std::make_shared<std::vector<T>>(std::move(value_segment->values()));
        std::sort(_dictionary->begin(), _dictionary->end());
        auto begin_erase_iter = std::unique(_dictionary->begin(), _dictionary->end());
        _dictionary->erase(begin_erase_iter, _dictionary->cend());
    }

    template<typename T>
    void DictionarySegment<T>::_init_attribute_vector(const std::shared_ptr<ValueSegment<T>> &value_segment) {
        if (value_segment->size() <= std::numeric_limits<uint8_t>::max()) {
            _attribute_vector = std::make_shared<FittedAttributeVector<uint8_t >>(value_segment->size());
        } else if (value_segment->size() <= std::numeric_limits<uint16_t>::max()) {
            _attribute_vector = std::make_shared<FittedAttributeVector<uint16_t >>(value_segment->size());
        } else {
            _attribute_vector = std::make_shared<FittedAttributeVector<uint32_t >>(value_segment->size());
        }

        ChunkOffset att_vec_index = 0;
        for (const auto &value : value_segment->values()) {
            const auto occurrence_iter = std::find(_dictionary->cbegin(), _dictionary->cend(), value);
            DebugAssert(occurrence_iter != _dictionary->cend(), "Value is not available in the dictionary.");
            uint32_t dict_pos = std::distance(_dictionary->cbegin(), occurrence_iter);
            _attribute_vector->set(att_vec_index, ValueID{dict_pos});
            ++att_vec_index;
        }
    }

    EXPLICITLY_INSTANTIATE_DATA_TYPES(DictionarySegment);

}  // namespace opossum

