#include "storage_manager.hpp"

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "utils/assert.hpp"

namespace opossum {

StorageManager& StorageManager::get() {
  static StorageManager storage_manager;
  return storage_manager;
}

void StorageManager::add_table(const std::string& name, std::shared_ptr<Table> table) {
  DebugAssert(!has_table(name), "The table " + name + " already exists.");
  _tables.emplace(name, table);
}

void StorageManager::drop_table(const std::string& name) {
  DebugAssert(has_table(name), "Table " + name + " not found.");
  _tables.erase(name);
}

std::shared_ptr<Table> StorageManager::get_table(const std::string& name) const {
  DebugAssert(has_table(name), "Table " + name + " not found.");
  return _tables.at(name);
}

bool StorageManager::has_table(const std::string& name) const {
  const auto map_iter = _tables.find(name);
  return map_iter != _tables.cend();
}

std::vector<std::string> StorageManager::table_names() const {
  std::vector<std::string> names;
  std::transform(_tables.cbegin(), _tables.cend(), std::back_inserter(names),
                 [](const auto& kvp) { return kvp.first; });
  return names;
}

void StorageManager::print(std::ostream& out) const {
  for (const auto& [tbl_name, table] : _tables) {
    out << tbl_name << ", " << table->column_count() << ", " << table->row_count() << ", " << table->chunk_count();
    out << std::endl;
  }
}

void StorageManager::reset() { _tables.clear(); }

}  // namespace opossum
