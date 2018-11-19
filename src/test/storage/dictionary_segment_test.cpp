#include <limits>
#include <memory>
#include <string>

#include "../lib/type_cast.hpp"
#include "gtest/gtest.h"

#include "../../lib/resolve_type.hpp"
#include "../../lib/storage/base_segment.hpp"
#include "../../lib/storage/dictionary_segment.hpp"
#include "../../lib/storage/value_segment.hpp"
#include "../base_test.hpp"

class StorageDictionarySegmentTest : public opossum::BaseTest {
 protected:
  std::shared_ptr<opossum::ValueSegment<int>> vc_int = std::make_shared<opossum::ValueSegment<int>>();
  std::shared_ptr<opossum::ValueSegment<std::string>> vc_str = std::make_shared<opossum::ValueSegment<std::string>>();
};

TEST_F(StorageDictionarySegmentTest, CompressSegmentString) {
  vc_str->append("Bill");
  vc_str->append("Steve");
  vc_str->append("Alexander");
  vc_str->append("Steve");
  vc_str->append("Hasso");
  vc_str->append("Bill");

  auto col = opossum::make_shared_by_data_type<opossum::BaseSegment, opossum::DictionarySegment>("string", vc_str);
  auto dict_col = std::dynamic_pointer_cast<opossum::DictionarySegment<std::string>>(col);

  // Test attribute_vector size
  EXPECT_EQ(dict_col->size(), 6u);

  // Test dictionary size (uniqueness)
  EXPECT_EQ(dict_col->unique_values_count(), 4u);

  // Test sorting
  auto dict = dict_col->dictionary();
  EXPECT_EQ((*dict)[0], "Alexander");
  EXPECT_EQ((*dict)[1], "Bill");
  EXPECT_EQ((*dict)[2], "Hasso");
  EXPECT_EQ((*dict)[3], "Steve");
}

TEST_F(StorageDictionarySegmentTest, LowerUpperBound) {
  for (int i = 0; i <= 10; i += 2) {
    vc_int->append(i);
  }

  auto col = opossum::make_shared_by_data_type<opossum::BaseSegment, opossum::DictionarySegment>("int", vc_int);
  auto dict_col = std::dynamic_pointer_cast<opossum::DictionarySegment<int>>(col);

  EXPECT_EQ(dict_col->lower_bound(4), (opossum::ValueID)2);
  EXPECT_EQ(dict_col->upper_bound(4), (opossum::ValueID)3);

  EXPECT_EQ(dict_col->lower_bound(5), (opossum::ValueID)3);
  EXPECT_EQ(dict_col->upper_bound(5), (opossum::ValueID)3);

  EXPECT_EQ(dict_col->lower_bound(opossum::AllTypeVariant{4}), (opossum::ValueID)2);
  EXPECT_EQ(dict_col->upper_bound(opossum::AllTypeVariant{4}), (opossum::ValueID)3);

  EXPECT_EQ(dict_col->lower_bound(opossum::AllTypeVariant{5}), (opossum::ValueID)3);
  EXPECT_EQ(dict_col->upper_bound(opossum::AllTypeVariant{5}), (opossum::ValueID)3);

  EXPECT_EQ(dict_col->lower_bound(15), opossum::INVALID_VALUE_ID);
  EXPECT_EQ(dict_col->upper_bound(15), opossum::INVALID_VALUE_ID);
}

TEST_F(StorageDictionarySegmentTest, GetValue) {
  for (int i = 0; i <= 10; i += 2) {
    vc_int->append(i);
  }
  auto col = opossum::make_shared_by_data_type<opossum::BaseSegment, opossum::DictionarySegment>("int", vc_int);
  auto dict_col = std::dynamic_pointer_cast<opossum::DictionarySegment<int>>(col);

  EXPECT_EQ(dict_col->get(1), 2);
  EXPECT_EQ(dict_col->operator[](1), opossum::AllTypeVariant(2));
}

TEST_F(StorageDictionarySegmentTest, InitUint16) {
  for (int i = 0; i < std::numeric_limits<uint8_t>::max() + 1; ++i) {
    vc_int->append(i);
  }
  const auto col = opossum::make_shared_by_data_type<opossum::BaseSegment, opossum::DictionarySegment>("int", vc_int);
  const auto dict_col = std::static_pointer_cast<opossum::DictionarySegment<uint16_t>>(col);
  EXPECT_EQ(dict_col->attribute_vector()->width(), 2);
}

TEST_F(StorageDictionarySegmentTest, InitUint32) {
  for (int i = 0; i < std::numeric_limits<uint16_t>::max() + 1; ++i) {
    vc_int->append(i);
  }
  const auto col = opossum::make_shared_by_data_type<opossum::BaseSegment, opossum::DictionarySegment>("int", vc_int);
  const auto dict_col = std::static_pointer_cast<opossum::DictionarySegment<uint32_t>>(col);
  EXPECT_EQ(dict_col->attribute_vector()->width(), 4);
}

TEST_F(StorageDictionarySegmentTest, DictionarySegmentAccess) {
  vc_int->append(10);
  vc_int->append(50);
  auto col = opossum::make_shared_by_data_type<opossum::BaseSegment, opossum::DictionarySegment>("int", vc_int);
  auto dict_col = std::dynamic_pointer_cast<opossum::DictionarySegment<int>>(col);

  EXPECT_EQ(dict_col->get(0), 10);
  EXPECT_EQ(opossum::type_cast<int>(dict_col->operator[](1)), 50);
}

TEST_F(StorageDictionarySegmentTest, DictionarySegmentIsImmutable) {
  auto col = opossum::make_shared_by_data_type<opossum::BaseSegment, opossum::DictionarySegment>("int", vc_int);
  auto dict_col = std::dynamic_pointer_cast<opossum::DictionarySegment<int>>(col);

  EXPECT_THROW(dict_col->append({}), std::runtime_error);
}
