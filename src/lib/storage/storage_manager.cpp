#include "storage_manager.hpp"

#include <algorithm>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "table.hpp"

#include "utils/assert.hpp"

namespace opossum {

StorageManager& StorageManager::get() {
  static StorageManager storage_manager;
  return storage_manager;
}

void StorageManager::add_table(const std::string& name, std::shared_ptr<Table> table) {
  if (!_tables.emplace(name, table).second) {
    throw std::runtime_error("The table " + name + " already exists.");
  }
}

void StorageManager::drop_table(const std::string& name) {
  if (!_tables.erase(name)) {
    throw std::runtime_error("Table " + name + " not found.");
  }
}

std::shared_ptr<Table> StorageManager::get_table(const std::string& name) const {
  DebugAssert(has_table(name), "Table " + name + " not found.");
  return _tables.at(name);
}

bool StorageManager::has_table(const std::string& name) const { return _tables.find(name) != _tables.cend(); }

std::vector<std::string> StorageManager::table_names() const {
  std::vector<std::string> names;
  names.reserve(_tables.size());
  std::transform(_tables.cbegin(), _tables.cend(), std::back_inserter(names),
                 [](const auto& key_value_pair) { return key_value_pair.first; });
  return names;
}

void StorageManager::print(std::ostream& out) const {
  for (const auto& [table_name, table] : _tables) {
    out << "name[" << table_name << "], #columns[" << table->column_count() << "], #rows[" << table->row_count()
        << "], #chunks[" << table->chunk_count() << "]" << std::endl;
  }
}

void StorageManager::reset() { _tables.clear(); }

}  // namespace opossum
