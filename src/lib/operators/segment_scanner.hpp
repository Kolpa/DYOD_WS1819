#pragma once

#include <limits>
#include <memory>

#include "types.hpp"

#include "storage/dictionary_segment.hpp"
#include "storage/reference_segment.hpp"
#include "storage/storage_manager.hpp"
#include "storage/value_segment.hpp"

namespace opossum {

/**
 * This class allows to iterate over all unsigned integers in an interval.
 * Our use case is to iterate over an array.
 */
class ContinuousIndexFetcher {
 public:
  /**
   * Initializes a new index fetcher in the interval [0, size_t::max)
   */
  ContinuousIndexFetcher() : ContinuousIndexFetcher(0, std::numeric_limits<size_t>::max()) {}

  /**
   * Initializes a new index fetcher in the interval [start_index, end_index)
   */
  explicit ContinuousIndexFetcher(size_t start_index, size_t end_index)
      : start_index{start_index}, end_index{end_index}, _current{start_index - 1} {};

  const size_t start_index;
  const size_t end_index;

  /**
   * Returns the next index. Increases current() by one.
   */
  size_t next() { return ++_current; }

  bool has_next() { return _current + 1 < end_index; }

  size_t current() { return _current; }

 protected:
  size_t _current;
};

/**
 * The PosListIndexFetcher allows to iterate over a PosList in a given interval [start_index, end_index).
 * It returns the value in the given PosList at the current index.
 */
class PosListIndexFetcher {
 public:
  /**
   * Initializes a new PosListIndexFetcher for a given PosList.
   */
  explicit PosListIndexFetcher(size_t start_index, size_t end_index, const PosList& pos_list)
      : start_index{start_index}, end_index{end_index}, _current{start_index - 1}, _pos_list{pos_list} {};

  const size_t start_index;
  const size_t end_index;

  /**
   * Returns the value in PosList at the next index and increases the internal index.
   */
  size_t next() { return _pos_list[++_current].chunk_offset; }

  bool has_next() { return _current + 1 < end_index; }

  /**
   * Returns the value in PosList at the given index.
   */
  size_t current() { return _pos_list[_current].chunk_offset; }

 protected:
  size_t _current;
  const PosList& _pos_list;
};

template <typename T>
class EqualsScanner;

template <typename T>
class GreaterThanScanner;

template <typename T>
class GreaterThanEqualsScanner;

template <typename T>
class LessThanScanner;

template <typename T>
class LessThanEqualsScanner;

template <typename T>
class NotEqualsScanner;

/**
 * Scanner to select certain values in a variety of segments
 * @tparam T Type of the values the scanner will operate on,
 */
template <typename T>
class AbstractSegmentScanner : private Noncopyable {
 public:
  virtual ~AbstractSegmentScanner() = default;

  /**
   * Compares value with cmp_value. Let cmp be a binary comperator.
   * Returns true if "value cmp cmp_value" evaluates to true.
   */
  virtual bool compare(const T& value, const T& cmp_value) = 0;

  /**
   * Scans segment and returns a PosList with all RowsIds for which the compare function
   * yields true.
   * chunk_id is ignored if segment is a ReferenceSegment
   */
  PosList scan(const ChunkID chunk_id, const std::shared_ptr<BaseSegment>& segment, const T& cmp_value) {
    // Determine dynamic type of segment and forward to specialized scan implementation.
    if (const auto& reference_segment = std::dynamic_pointer_cast<ReferenceSegment>(segment)) {
      return scan(*reference_segment, cmp_value);
    } else if (const auto& value_segment = std::dynamic_pointer_cast<ValueSegment<T>>(segment)) {
      return scan(chunk_id, *value_segment, cmp_value);
    } else if (const auto& dictionary_segment = std::dynamic_pointer_cast<DictionarySegment<T>>(segment)) {
      return scan(chunk_id, *dictionary_segment, cmp_value);
    }

    throw std::runtime_error("Unsupported segment type.");
  }

  /**
   * Scans a DictionarySegment and returns a PosList with all RowsIds for which the compare function
   * yields true.
   */
  PosList scan(const ChunkID chunk_id, const DictionarySegment<T>& segment, const T& cmp_value) {
    auto index_fetcher = ContinuousIndexFetcher(0, segment.size());
    return this->template scan<ContinuousIndexFetcher>(chunk_id, segment, cmp_value, index_fetcher);
  }

  /**
   * Scans a ValueSegment and returns a PosList with all RowsIds for which the compare function
   * yields true.
   */
  PosList scan(const ChunkID chunk_id, const ValueSegment<T>& segment, const T& cmp_value) {
    auto index_fetcher = ContinuousIndexFetcher(0, segment.size());
    return this->template scan<ContinuousIndexFetcher>(chunk_id, segment, cmp_value, index_fetcher);
  }

  /**
   * Scans a ReferenceSegment and returns a PosList with all RowsIds for which the compare function
   * yields true.
   */
  PosList scan(const ReferenceSegment& segment, const T& cmp_value) {
    const auto& pos_list = *segment.pos_list();
    if (pos_list.empty()) {
      return PosList{};
    }

    const auto& table = segment.referenced_table();
    // PosList to hold the selected RowIDs
    PosList result;

    size_t start_index = 0;
    auto last_chunk_id = pos_list[0].chunk_id;

    // We imply that the PosList is sorted by chunk id. If not, a performance penalty could be observed.
    // Iterate over pos_list() of segment and search for a continuous sequence of RowIds with the same
    // chunk id. As soon as the chunk ids of two neighboring elements within pos_list() do not match
    // scan the segment corresponding to the chunk id. This is neccessary to optimize the value
    // selection within the segment which can be either a Value- or DictionarySegment.
    for (size_t index = 1; index < pos_list.size(); ++index) {
      const auto& row_id = pos_list[index];
      if (row_id.chunk_id != last_chunk_id) {
        const auto& chunk = table->get_chunk(last_chunk_id);
        const auto base_segment = chunk.get_segment(segment.referenced_column_id());
        const auto tmp_result = scan(last_chunk_id, base_segment, cmp_value, pos_list, start_index, index);
        // Move elements from tmp_result to the end of result
        result.insert(result.end(), std::make_move_iterator(tmp_result.begin()),
                      std::make_move_iterator(tmp_result.end()));
        start_index = index;
        last_chunk_id = row_id.chunk_id;
      }
    }

    // Exact duplicate to lines above include the entries in pos_list() belonging to the last chunk.
    {
      const auto& chunk = table->get_chunk(last_chunk_id);
      const auto base_segment = chunk.get_segment(segment.referenced_column_id());
      const auto tmp_result = scan(last_chunk_id, base_segment, cmp_value, pos_list, start_index, pos_list.size());
      // Move elements from tmp_result to the end of result
      result.insert(result.end(), std::make_move_iterator(tmp_result.begin()),
                    std::make_move_iterator(tmp_result.end()));
    }

    return result;
  }

  /**
   * Helper function to create the correct scanner given a scan_type.
   */
  static std::unique_ptr<AbstractSegmentScanner> from_scan_type(ScanType scan_type) {
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

  /**
   * Gets a value id to a value within a DictionarySegment, depending on the concrete implementation
   */
  virtual ValueID get_value_id(const DictionarySegment<T>& segment, const T& value) = 0;

  /**
   * Scans a BaseSegment
   * @param chunk_id ChunkId to segment which is scanned.
   * @param segment Segment to be scanned
   * @param cmp_value Value to comparte values to
   * @param pos_list List of positions to evaluate
   * @param start_index start index within pos_list
   * @param end_index end_index (excluding) within pos_list
   * @return A PosList with the selected Rows
   */
  PosList scan(const ChunkID chunk_id, const std::shared_ptr<BaseSegment>& segment, const T& cmp_value,
               const PosList& pos_list, size_t start_index, size_t end_index) {
    auto index_fetcher = PosListIndexFetcher(start_index, end_index, pos_list);
    // Determine dynamic type of segment and forward to concrete implementation.
    if (const auto& value_segment = std::dynamic_pointer_cast<ValueSegment<T>>(segment)) {
      return this->template scan<PosListIndexFetcher>(chunk_id, *value_segment, cmp_value, index_fetcher);
    } else if (const auto& dictionary_segment = std::dynamic_pointer_cast<DictionarySegment<T>>(segment)) {
      return this->template scan<PosListIndexFetcher>(chunk_id, *dictionary_segment, cmp_value, index_fetcher);
    }
    throw std::runtime_error("Unsupported segment type.");
  }

  /**
   * Concrete implementation for scanning a DictionarySegment.
   * @tparam IndexFetcher Type of index fetcher to use. This way we don't have to duplicate our code
   * to iterate over a ReferenceSegment or DictionarySegment.
   */
  template <typename IndexFetcher>
  PosList scan(const ChunkID chunk_id, const DictionarySegment<T>& segment, const T& cmp_value,
               IndexFetcher& index_fetcher) {
    const auto value_id_to_compare_to = get_value_id(segment, cmp_value);

    // Todo: There are cases where we could have a select all or select none case here,
    // for example if a value cannot be found within the dictionary.
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

  /**
   * Concrete implementation for scanning a ValueSegment.
   * @tparam IndexFetcher Type of index fetcher to use. This way we don't have to duplicate our code
   * to iterate over a ReferenceSegment or ValueSegment.
   */
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

template <typename T>
class LessThanScanner : public AbstractSegmentScanner<T> {
 public:
  bool compare(const T& value, const T& cmp_value) override { return value < cmp_value; }

 protected:
  /**
   * Returns the value_id to a value which is the first value >= cmp_value
   */
  ValueID get_value_id(const DictionarySegment<T>& segment, const T& cmp_value) override {
    return segment.lower_bound(cmp_value);
  }

  bool compare_by_value_id(const ValueID& value_id, const ValueID& cmp_value_value_id) override {
    return value_id < cmp_value_value_id;
  };
};

template <typename T>
class LessThanEqualsScanner : public AbstractSegmentScanner<T> {
 public:
  bool compare(const T& value, const T& cmp_value) override { return value <= cmp_value; }

 protected:
  /**
  * Returns the value_id to a value which is the first value > cmp_value
  */
  ValueID get_value_id(const DictionarySegment<T>& segment, const T& cmp_value) override {
    return segment.upper_bound(cmp_value);
  }

  bool compare_by_value_id(const ValueID& value_id, const ValueID& cmp_value_value_id) override {
    return value_id < cmp_value_value_id;
  }
};

template <typename T>
class EqualsScanner : public AbstractSegmentScanner<T> {
 public:
  bool compare(const T& value, const T& cmp_value) override { return value == cmp_value; }

 protected:
  /**
  * Returns the value_id to cmp_value. Returns INVALID_VALUE_ID if cmp_value could not be found.
  */
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

template <typename T>
class NotEqualsScanner : public AbstractSegmentScanner<T> {
 public:
  bool compare(const T& value, const T& cmp_value) override { return value != cmp_value; }

 protected:
  /**
  * Returns the value_id to cmp_value. Returns INVALID_VALUE_ID if cmp_value could not be found.
  */
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

template <typename T>
class GreaterThanScanner : public AbstractSegmentScanner<T> {
 public:
  bool compare(const T& value, const T& cmp_value) override { return value > cmp_value; }

 protected:
  /**
  * Returns the value_id to a value which is the first value > cmp_value
  */
  ValueID get_value_id(const DictionarySegment<T>& segment, const T& cmp_value) override {
    const auto value_id = segment.upper_bound(cmp_value);
    return value_id;
  }

  bool compare_by_value_id(const ValueID& value_id, const ValueID& cmp_value_value_id) override {
    return value_id >= cmp_value_value_id;
  }
};

template <typename T>
class GreaterThanEqualsScanner : public AbstractSegmentScanner<T> {
 public:
  bool compare(const T& value, const T& cmp_value) override { return value >= cmp_value; }

 protected:
  /**
  * Returns the value_id to a value which is the first value >= cmp_value
  */
  ValueID get_value_id(const DictionarySegment<T>& segment, const T& cmp_value) override {
    const auto value_id = segment.lower_bound(cmp_value);
    return value_id;
  }

  bool compare_by_value_id(const ValueID& value_id, const ValueID& cmp_value_value_id) override {
    return value_id >= cmp_value_value_id;
  }
};

}  // namespace opossum
