#include <iostream>

#include <memory>
#include <storage/table.hpp>
#include <storage/fitted_attribute_vector.hpp>
#include <storage/dictionary_segment.hpp>
#include "../lib/all_type_variant.hpp"
#include "storage/value_segment.hpp"
#include "../lib/utils/assert.hpp"



int main() {
//  opossum::Assert(true, "We can use opossum files here :)");
//  opossum::FittedAttributeVector<uint8_t> f1;
//  std::cout << static_cast<uint32_t>(f1.width()) << std::endl;
//  opossum::FittedAttributeVector<uint16_t> f2;
//  std::cout << static_cast<uint32_t>(f2.width()) << std::endl;
//  opossum::FittedAttributeVector<uint32_t> f3;
//  std::cout << static_cast<uint32_t>(f3.width()) << std::endl;
//  opossum::FittedAttributeVector<uint64_t> f4;
//  std::cout << static_cast<uint32_t>(f4.width()) << std::endl;

  auto int_vs = std::make_shared<opossum::ValueSegment<int>>();
  int_vs->append(5);
  int_vs->append(1);
  int_vs->append(2);

  auto ds = std::make_shared<opossum::DictionarySegment<int>>(int_vs);
  std::cout << ds->lower_bound(1) << std::endl;

  return 0;
}
