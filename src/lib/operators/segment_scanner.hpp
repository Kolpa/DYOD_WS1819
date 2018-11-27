#pragma once

#include <limits>
#include <memory>

#include "types.hpp"

#include "storage/dictionary_segment.hpp"
#include "storage/reference_segment.hpp"
#include "storage/value_segment.hpp"

namespace opossum {


class ContinuousIndexFetcher {
 public:
  ContinuousIndexFetcher() : ContinuousIndexFetcher(0, std::numeric_limits<size_t>::max()) {};

  ContinuousIndexFetcher(size_t start_index, size_t end_index)
      : start_index{start_index}, end_index{end_index}, _current{start_index - 1};

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
  PosListIndexFetcher(size_t start_index, size_t end_index, const PosList& pos_list)
      : start_index{start_index}, end_index{end_index}, _current{start_index - 1}, _pos_list{pos_list};

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
class AbstractScanner {

 public:

  virtual ~AbstractScanner() = default;

  virtual bool compare(const T& value, const T& cmp_value) = 0;

  PosList scan(const ChunkID chunk_id, const std::shared_ptr<DictionarySegment<T>>& segment, const T& cmp_value) {
    return scan<ContinuousIndexFetcher>(chunk_id, segment, cmp_value, ContinuousIndexFetcher(0, segment->size()));
  }

  PosList scan(const ChunkID chunk_id, const std::shared_ptr<ValueSegment<T>>& segment, const T& cmp_value) {
    return scan<ContinuousIndexFetcher>(chunk_id, segment, cmp_value, ContinuousIndexFetcher(0, segment->size()));
  }

  PosList scan(const ChunkID chunk_id, const std::shared_ptr<ReferenceSegment>& segment, const T& cmp_value) {
    // Marcells Code hier unter Verwendung des PosListIndexFetcher;
  }

 protected:

  virtual bool compare_by_value_id(const ValueID& value_id, const ValueID& cmp_value_value_id) = 0;

  virtual ValueID get_value_id(const std::shared_ptr<DictionarySegment<T>>& segment, const T& value) = 0;

  template <typename IndexFetcher>
  PosList scan(const ChunkID chunk_id, const std::shared_ptr<DictionarySegment<T>>& segment, const T& cmp_value,
               IndexFetcher& index_fetcher) {

    const auto value_id_to_compare_to = get_value_id(segment, cmp_value);

    // Todo: there are cases where we could have a select all or select none case here.
    // say a avlue is not in the dictionary.
    PosList pos_list;

    const auto& attribute_vector = segment->attribute_vector();

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
  PosList scan(const ChunkID chunk_id, const std::shared_ptr<ValueSegment<T>>& segment, const T& cmp_value,
               IndexFetcher& index_fetcher) {

    PosList pos_list;

    const auto& values = segment->values();
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
  ValueID get_value_id(const std::shared_ptr<DictionarySegment<T>>& segment, const T& cmp_value) override {
    return segment->lower_bound(cmp_value);
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
  ValueID get_value_id(const std::shared_ptr<DictionarySegment<T>>& segment, const T& cmp_value) override {
    return segment->upper_bound(cmp_value);
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
  ValueID get_value_id(const std::shared_ptr<DictionarySegment<T>>& segment, const T& cmp_value) override {
    auto value_id = segment->lower_bound(cmp_value);
    if (value_id != INVALID_VALUE_ID && cmp_value != segment->dictionary()[value_id]) {
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
  ValueID get_value_id(const std::shared_ptr<DictionarySegment<T>>& segment, const T& cmp_value) override {
    auto value_id = segment->lower_bound(cmp_value);
    if (value_id != INVALID_VALUE_ID && cmp_value != segment->dictionary()[value_id]) {
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
  ValueID get_value_id(const std::shared_ptr<DictionarySegment<T>>& segment, const T& cmp_value) override {
    const auto value_id = segment->upper_bound(cmp_value);
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
  ValueID get_value_id(const std::shared_ptr<DictionarySegment<T>>& segment, const T& cmp_value) override {
    const auto value_id = segment->lower_bound(cmp_value);
    return value_id;
  }

  bool compare_by_value_id(const ValueID& value_id, const ValueID& cmp_value_value_id) override {
    return value_id >= cmp_value_value_id;
  }
};




} // namespace opossum