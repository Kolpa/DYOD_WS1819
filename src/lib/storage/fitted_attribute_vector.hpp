#pragma once

#include "base_attribute_vector.hpp"
#include "types.hpp"

namespace opossum {

    class FittedAttributeVector : public BaseAttributeVector {
      public:
        ValueID get(const size_t i) const override;

        void set(const size_t i, const ValueID value_Id) override;

        size_t size() const override;

        AttributeVectorWidth width() const override;

      protected:
        std::vector<ValueID> _value_ids;
    };
}