#pragma once

#include <limits>
#include <memory>

#include "types.hpp"

#include "storage/dictionary_segment.hpp"
#include "storage/reference_segment.hpp"
#include "storage/value_segment.hpp"

namespace opossum {

class AbstractIndexFetcher
{
 public:
  virtual ~AbstractIndexFetcher() = default;
  virtual size_t current();
  virtual size_t next() = 0;
  virtual bool has_next() = 0;
};

class ContinuousIndexFetcher : public AbstractIndexFetcher
{
 public:
  ContinuousIndexFetcher() : ContinuousIndexFetcher(0, std::numeric_limits<size_t>::max()) {};

  ContinuousIndexFetcher(size_t start_index, size_t end_index) : start_index{start_index}, end_index{end_index},
    _current{start_index - 1};

  const size_t start_index;
  const size_t end_index;

  size_t next() override {
    return ++_current;
  }

  bool has_next() override {
    return _current + 1 < end_index;
  };

  size_t current() override {
    return _current;
  }

 protected:
  size_t _current;
};

class PosListIndexFetcher : public AbstractIndexFetcher
{
  PosListIndexFetcher(size_t start_index, size_t end_index, const PosList& pos_list) : start_index{start_index}, end_index{end_index},
  _current{start_index - 1}, _pos_list{pos_list};

  const size_t start_index;
  const size_t end_index;

  size_t next() override {
    return _pos_list[++_current].chunk_offset;
  }

  bool has_next() override {
    return _current + 1 < end_index;
  };

  size_t current() override {
    return _pos_list[_current].chunk_offset;
  }

 protected:
  size_t _current;
  const PosList& _pos_list;
};

template<typename T>
class abstract_scan_type {

 public:

  virtual ~abstract_scan_type() = default;

  virtual bool compare(const T& value, const T& cmp_value) = 0;

  virtual PosList scan(const ChunkID chunk_id, const std::shared_ptr<DictionarySegment<T>>& segment, const T& cmp_value) {
    return scan(chunk_id, segment, cmp_value, new ContinuousIndexFetcher(0, segment->size()));
  }

  PosList scan(const ChunkID chunk_id, const std::shared_ptr<ValueSegment<T>>& segment, const T& cmp_value) {
    return scan(chunk_id, segment, cmp_value, new ContinuousIndexFetcher(0, segment->size()));
  }

  PosList scan(const ChunkID chunk_id, const std::shared_ptr<ReferenceSegment>& segment, const T& cmp_value) {
    // Marcells Code hier unter Verwendung des PosListIndexFetcher;
  }

 protected:

  virtual bool compare(const ValueID& value_id, const ValueID& cmp_value_value_id) = 0;

  virtual ValueID get_value_id_to_cmp_value(const std::shared_ptr<DictionarySegment<T>>& segment, const T& cmp_value) = 0;

  PosList scan(const ChunkID chunk_id, const std::shared_ptr<DictionarySegment<T>>& segment, const T& cmp_value,
      AbstractIndexFetcher& index_fetcher) {

    auto value_id_to_compare_to = get_value_id_to_cmp_value(segment, cmp_value);
    PosList pos_list;
    const auto& attribute_vector = segment->attribute_vector();

    while (index_fetcher.has_next()) {
      const auto index = index_fetcher.next();
      const auto& value_id = attribute_vector->get(index);
      if (compare(value_id, value_id_to_compare_to)) {
        pos_list.emplace_back(RowID{chunk_id, ChunkOffset(index)});
      }
    }

    return pos_list;
  }

  PosList scan(const ChunkID chunk_id, const std::shared_ptr<ValueSegment<T>>& segment, const T& cmp_value,
    AbstractIndexFetcher& index_fetcher) {

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

template <typename T>
class LessThanScanner : public abstract_scan_type<T> {

 public:
  bool compare(const T& value, const T& cmp_value) override {
    return value < cmp_value;
  }

 protected:
  ValueID get_value_id_to_cmp_value(const std::shared_ptr<DictionarySegment<T>>& segment, const T& cmp_value) override {
    return segment->lower_bound(cmp_value);
  }

  bool compare(const ValueID& value_id, const ValueID& cmp_value_value_id) override {
    return value_id < cmp_value_value_id;
  };



};



} // namespace opossum