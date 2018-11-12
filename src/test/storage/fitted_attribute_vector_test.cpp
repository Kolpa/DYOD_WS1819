#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../base_test.hpp"
#include "gtest/gtest.h"

#include "../lib/storage/fitted_attribute_vector.hpp"

namespace opossum {

class FittedAttributeVectorTest : public BaseTest {
 protected:
  void SetUp() override {
    std::vector<uint8_t> base_data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    attr_vect = std::make_shared<FittedAttributeVector<uint8_t>>(std::move(base_data));
  }
  std::shared_ptr<FittedAttributeVector<uint8_t>> attr_vect;
};

TEST_F(FittedAttributeVectorTest, SetAndGetVectorValueID) {
  EXPECT_EQ(attr_vect->get(0), (opossum::ValueID)1);
  attr_vect->set(0, (opossum::ValueID)2);
  EXPECT_EQ(attr_vect->get(0), (opossum::ValueID)2);
}

TEST_F(FittedAttributeVectorTest, GetVectorWidth) { EXPECT_EQ(attr_vect->width(), sizeof(uint8_t)); }

}  // namespace opossum
