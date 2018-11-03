#pragma once

#include "base_attribute_vector.hpp"
#include "types.hpp"
#include "utils/assert.hpp"

namespace opossum {

    template <class T>
    class FittedAttributeVector : public BaseAttributeVector {
      public:

        explicit FittedAttributeVector();
        // returns the value at a given position
        ValueID get(const ChunkOffset i) const override;

        // sets the value_id at a given position
        void set(const ChunkOffset i, const ValueID value_Id) override;

        // returns the number of values
        ChunkOffset size() const override;

        // returns the width of the values in bytes
        AttributeVectorWidth width() const override;

      protected:
        std::vector<T> _value_ids;
    };

    template <class T>
    FittedAttributeVector<T>::FittedAttributeVector() {
        bool valid_type = std::is_same_v<T, uint8_t>  || std::is_same_v<T, uint16_t> || std::is_same_v<T, uint32_t>;
        DebugAssert(valid_type, "Template type has to be uint8_t, uint16_t or uint32_t");
    }

    template <class T>
    ValueID FittedAttributeVector<T>::get(const ChunkOffset i) const {
        return ValueID{static_cast<ValueID>(_value_ids[i])};
    }

    template <class T>
    void FittedAttributeVector<T>::set(const ChunkOffset i, const ValueID value_Id) {
        _value_ids[i] = value_Id;
    }

    template <class T>
    ChunkOffset FittedAttributeVector<T>::size() const {
        return _value_ids.size();
    }

    template <class T>
    AttributeVectorWidth FittedAttributeVector<T>::width() const {
        return sizeof(T);
    }
}