#include <memory>
#include <string>
#include <vector>

#include "../base_test.hpp"
#include "gtest/gtest.h"

#include "../lib/resolve_type.hpp"
#include "../lib/storage/base_segment.hpp"
#include "../lib/storage/chunk.hpp"
#include "../lib/storage/fitted_attribute_vector.hpp"
#include "../lib/types.hpp"  //

class FittedAttributeVectorTest : public opossum::BaseTest {
 protected:
  std::shared_ptr<opossum::FittedAttributeVector<uint8_t>> fitted_att_vec =
      std::make_shared<opossum::FittedAttributeVector<uint8_t>>(std::vector<uint8_t>{1, 2, 3});
};

TEST_F(FittedAttributeVectorTest, Get) {
  EXPECT_EQ(fitted_att_vec->get(1), opossum::ValueID{2});
  EXPECT_THROW(fitted_att_vec->get(10), std::logic_error);
}

TEST_F(FittedAttributeVectorTest, Set) {
  fitted_att_vec->set(0, opossum::ValueID{5});
  fitted_att_vec->set(20, opossum::ValueID{13});
  EXPECT_EQ(fitted_att_vec->get(0), opossum::ValueID{5});
  EXPECT_EQ(fitted_att_vec->get(20), opossum::ValueID{13});
  EXPECT_EQ(fitted_att_vec->get(19), opossum::ValueID{0});
}

TEST_F(FittedAttributeVectorTest, Size) { EXPECT_EQ(fitted_att_vec->size(), 3u); }

TEST_F(FittedAttributeVectorTest, Width) { EXPECT_EQ(fitted_att_vec->width(), opossum::AttributeVectorWidth{sizeof(uint8_t)}); }

