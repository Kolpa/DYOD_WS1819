#include <cstdlib>
#include <memory>
#include <string>

#include "../base_test.hpp"
#include "gtest/gtest.h"

#include "../lib/type_cast.hpp"
#include "../lib/types.hpp"

#include "utils/load_table.hpp"

namespace opossum {

class LoadTableTest : public BaseTest {};

TEST_F(LoadTableTest, loadTable) {
  std::shared_ptr<Table> t = load_table("./src/test/tables/int_float.tbl", 2);
  std::shared_ptr<Table> t_expected = std::make_shared<Table>(2);
  t_expected->add_column("a", "int");
  t_expected->add_column("b", "float");
  t_expected->append({12345, 458.7f});
  t_expected->append({123, 456.7f});
  t_expected->append({1234, 457.7f});
  EXPECT_TABLE_EQ(t, t_expected);
}

}  // namespace opossum
