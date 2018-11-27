#pragma once

#include <limits>
#include <memory>

#include "types.hpp"

#include "storage/dictionary_segment.hpp"
#include "storage/reference_segment.hpp"
#include "storage/storage_manager.hpp"
#include "storage/value_segment.hpp"

namespace opossum {


class ContinuousIndexFetcher {
 public:
  ContinuousIndexFetcher() : ContinuousIndexFetcher(0, std::numeric_limits<size_t>::max()) {};

  ContinuousIndexFetcher(size_t start_index, size_t end_index)
      : start_index{start_index}, end_index{end_index}, _current{start_index - 1} {};

  const size_t start_index;
  const size_t end_index;

  size_t next() {
    return ++_current;
  }

  bool has_next() {
    return _current + 1 < end_index;
  };

  size_t current() {
    return _current;
  }

 protected:
  size_t _current;
};

class PosListIndexFetcher {
 public:
  PosListIndexFetcher(size_t start_index, size_t end_index, const PosList& pos_list)
      : start_index{start_index}, end_index{end_index}, _current{start_index - 1}, _pos_list{pos_list} {};

  const size_t start_index;
  const size_t end_index;

  size_t next() {
    return _pos_list[++_current].chunk_offset;
  }

  bool has_next() {
    return _current + 1 < end_index;
  };

  size_t current() {
    return _pos_list[_current].chunk_offset;
  }

 protected:
  size_t _current;
  const PosList& _pos_list;
};

template<typename T>
class EqualsScanner;

template<typename T>
class GreaterThanScanner;

template<typename T>
class GreaterThanEqualsScanner;

template<typename T>
class LessThanScanner;

template<typename T>
class LessThanEqualsScanner;

template<typename T>
class NotEqualsScanner;

template<typename T>
class AbstractScanner {

 public:

  virtual ~AbstractScanner() = default;

  virtual bool compare(const T& value, const T& cmp_value) = 0;

  PosList scan(const ChunkID chunk_id, const std::shared_ptr<BaseSegment>& segment, const T& cmp_value) {
    if (const auto& reference_segment = std::dynamic_pointer_cast<ReferenceSegment>(segment)) {
      return scan(chunk_id, *reference_segment, cmp_value);
    }
    else if (const auto& value_segment = std::dynamic_pointer_cast<ValueSegment<T>>(segment)) {
      return scan(chunk_id, *value_segment, cmp_value);
    }
    else if (const auto& dictionary_segment = std::dynamic_pointer_cast<DictionarySegment<T>>(segment)) {
      return scan(chunk_id, *dictionary_segment, cmp_value);
    }

    throw std::runtime_error("Unsupported segment type.");
  }

  PosList scan(const ChunkID chunk_id, const DictionarySegment<T>& segment, const T& cmp_value) {
    auto index_fetcher = ContinuousIndexFetcher(0, segment.size());
    return this->template scan<ContinuousIndexFetcher>(chunk_id, segment, cmp_value, index_fetcher);
  }

  PosList scan(const ChunkID chunk_id, const ValueSegment<T>& segment, const T& cmp_value) {
    auto index_fetcher = ContinuousIndexFetcher(0, segment.size());
    return this->template scan<ContinuousIndexFetcher>(chunk_id, segment, cmp_value, index_fetcher);
  }

  PosList scan(const ChunkID chunk_id, const ReferenceSegment& segment, const T& cmp_value) {

    const auto& pos_list = *segment.pos_list();
    if (pos_list.empty()) {
      return PosList {};
    }

    const auto& table = segment.referenced_table();
    PosList result;

    size_t start_index = 0;
    auto last_chunk_id = pos_list[0].chunk_id;

    for (size_t index = 1; index < pos_list.size(); ++index){
      const auto& row_id = pos_list[index];
      if (row_id.chunk_id != last_chunk_id) {
        const auto& chunk = table->get_chunk(last_chunk_id);
        const auto base_segment = chunk.get_segment(segment.referenced_column_id());
        const auto tmp_result = scan(chunk_id, base_segment, cmp_value, pos_list, start_index, index);
        result.insert(result.end(), std::make_move_iterator(tmp_result.begin()), std::make_move_iterator(tmp_result.end()));
        start_index = index;
        last_chunk_id = row_id.chunk_id;
      }
    }

    { // Exact duplicate to lines above to add range from [start_index, pos_list.size())
      const auto& chunk = table->get_chunk(last_chunk_id);
      const auto base_segment = chunk.get_segment(segment.referenced_column_id());
      const auto tmp_result = scan(chunk_id, base_segment, cmp_value, pos_list, start_index, pos_list.size());
      result.insert(result.end(), std::make_move_iterator(tmp_result.begin()), std::make_move_iterator(tmp_result.end()));
    }

    return result;
  }

  static std::unique_ptr<AbstractScanner> from_scan_type(ScanType scan_type) {
    switch (scan_type) {
      case ScanType::OpEquals:
        return std::make_unique<EqualsScanner<T>>();
      case ScanType::OpGreaterThan:
        return std::make_unique<GreaterThanScanner<T>>();
      case ScanType::OpGreaterThanEquals:
        return std::make_unique<GreaterThanEqualsScanner<T>>();
      case ScanType::OpLessThan:
        return std::make_unique<LessThanScanner<T>>();
      case ScanType::OpLessThanEquals:
        return std::make_unique<LessThanEqualsScanner<T>>();
      case ScanType::OpNotEquals:
        return std::make_unique<NotEqualsScanner<T>>();
      default:
        throw std::runtime_error("comparison operator is not supported.");
    }
  }

 protected:

  virtual bool compare_by_value_id(const ValueID& value_id, const ValueID& cmp_value_value_id) = 0;

  virtual ValueID get_value_id(const DictionarySegment<T>& segment, const T& value) = 0;

  PosList scan(const ChunkID chunk_id, const std::shared_ptr<BaseSegment>& segment, const T& cmp_value, const PosList& pos_list, size_t start_index, size_t end_index) {
    auto index_fetcher = PosListIndexFetcher(start_index, end_index, pos_list);
    if (const auto& value_segment = std::dynamic_pointer_cast<ValueSegment<T>>(segment)) {
      return this->template scan<PosListIndexFetcher>(chunk_id, *value_segment, cmp_value, index_fetcher);
    }
    else if (const auto& dictionary_segment = std::dynamic_pointer_cast<DictionarySegment<T>>(segment)) {
      return this->template scan<PosListIndexFetcher>(chunk_id, *dictionary_segment, cmp_value, index_fetcher);
    }
    throw std::runtime_error("Unsupported segment type.");
  }

  template <typename IndexFetcher>
  PosList scan(const ChunkID chunk_id, const DictionarySegment<T>& segment, const T& cmp_value,
               IndexFetcher& index_fetcher) {

    const auto value_id_to_compare_to = get_value_id(segment, cmp_value);

    // Todo: there are cases where we could have a select all or select none case here.
    // say a avlue is not in the dictionary.
    PosList pos_list;

    const auto& attribute_vector = segment.attribute_vector();

    while (index_fetcher.has_next()) {
      const auto index = index_fetcher.next();
      const auto& value_id = attribute_vector->get(index);
      if (compare_by_value_id(value_id, value_id_to_compare_to)) {
        pos_list.emplace_back(RowID{chunk_id, ChunkOffset(index)});
      }
    }

    return pos_list;
  }

  template <typename IndexFetcher>
  PosList scan(const ChunkID chunk_id, const ValueSegment<T>& segment, const T& cmp_value,
               IndexFetcher& index_fetcher) {

    PosList pos_list;

    const auto& values = segment.values();
    while (index_fetcher.has_next()) {
      const auto index = index_fetcher.next();
      const T& value = values[index];
      if (compare(value, cmp_value)) {
        pos_list.emplace_back(RowID{chunk_id, ChunkOffset(index)});
      }
    }

    return pos_list;
  }

};

template<typename T>
class LessThanScanner : public AbstractScanner<T> {

 public:
  bool compare(const T& value, const T& cmp_value) override {
    return value < cmp_value;
  }

 protected:
  ValueID get_value_id(const DictionarySegment<T>& segment, const T& cmp_value) override {
    return segment.lower_bound(cmp_value);
  }

  bool compare_by_value_id(const ValueID& value_id, const ValueID& cmp_value_value_id) override {
    return value_id < cmp_value_value_id;
  };
};

template<typename T>
class LessThanEqualsScanner : public AbstractScanner<T> {

 public:
  bool compare(const T& value, const T& cmp_value) override {
    return value <= cmp_value;
  }

 protected:
  ValueID get_value_id(const DictionarySegment<T>& segment, const T& cmp_value) override {
    return segment.upper_bound(cmp_value);
  }

  bool compare_by_value_id(const ValueID& value_id, const ValueID& cmp_value_value_id) override {
    return value_id < cmp_value_value_id;
  }
};

template<typename T>
class EqualsScanner : public AbstractScanner<T> {

 public:
  bool compare(const T& value, const T& cmp_value) override {
    return value == cmp_value;
  }

 protected:
  ValueID get_value_id(const DictionarySegment<T>& segment, const T& cmp_value) override {
    auto value_id = segment.lower_bound(cmp_value);
    if (value_id != INVALID_VALUE_ID && cmp_value != segment.dictionary()->operator[](value_id)) {
      value_id = INVALID_VALUE_ID;
    }
    return value_id;
  }

  bool compare_by_value_id(const ValueID& value_id, const ValueID& cmp_value_value_id) override {
    return value_id == cmp_value_value_id;
  }
};

template<typename T>
class NotEqualsScanner : public AbstractScanner<T> {

 public:
  bool compare(const T& value, const T& cmp_value) override {
    return value != cmp_value;
  }

 protected:
  ValueID get_value_id(const DictionarySegment<T>& segment, const T& cmp_value) override {
    auto value_id = segment.lower_bound(cmp_value);
    if (value_id != INVALID_VALUE_ID && cmp_value != segment.dictionary()->operator[](value_id)) {
      value_id = INVALID_VALUE_ID;
    }

    return value_id;
  }

  bool compare_by_value_id(const ValueID& value_id, const ValueID& cmp_value_value_id) override {
    return value_id != cmp_value_value_id;
  }
};

template<typename T>
class GreaterThanScanner : public AbstractScanner<T> {

 public:
  bool compare(const T& value, const T& cmp_value) override {
    return value > cmp_value;
  }

 protected:
  ValueID get_value_id(const DictionarySegment<T>& segment, const T& cmp_value) override {
    const auto value_id = segment.upper_bound(cmp_value);
    return value_id;
  }

  bool compare_by_value_id(const ValueID& value_id, const ValueID& cmp_value_value_id) override {
    return value_id >= cmp_value_value_id;
  }
};

template<typename T>
class GreaterThanEqualsScanner : public AbstractScanner<T> {

 public:
  bool compare(const T& value, const T& cmp_value) override {
    return value >= cmp_value;
  }

 protected:
  ValueID get_value_id(const DictionarySegment<T>& segment, const T& cmp_value) override {
    const auto value_id = segment.lower_bound(cmp_value);
    return value_id;
  }

  bool compare_by_value_id(const ValueID& value_id, const ValueID& cmp_value_value_id) override {
    return value_id >= cmp_value_value_id;
  }
};




} // namespace opossum