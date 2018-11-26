#pragma once

#include <memory>
#include <utility>

#include "base_table_scan_impl.hpp"
#include "storage/reference_segment.hpp"
#include "type_cast.hpp"
#include "types.hpp"

#include "storage/chunk.hpp"
#include "storage/table.hpp"
#include "utils/assert.hpp"

#define POS_LIST_RESERVATION_FACTOR 0.5

namespace opossum {

template <typename T>
class TableScanImpl : public BaseTableScanImpl {
 public:
  TableScanImpl(const std::shared_ptr<const Table>& input_table, const ColumnID column_id, const ScanType scan_type,
                const AllTypeVariant search_value)
      : BaseTableScanImpl(),
        _input_table(input_table),
        _column_id(column_id),
        _scan_type(scan_type),
        _search_value(type_cast<T>(search_value)) {
    DebugAssert(input_table != nullptr, "Input table must be defined.");
  }

 protected:
  const std::shared_ptr<const Table> _input_table;
  const ColumnID _column_id;
  const ScanType _scan_type;
  const T _search_value;

  std::shared_ptr<const Table> execute() const override {
    auto output_table = std::make_shared<Table>();

    // initializes the schema of the output table.
    for (ColumnID column_index{0}; column_index < _input_table->column_count(); ++column_index) {
      output_table->add_column_definition(_input_table->column_name(column_index),
                                          _input_table->column_type(column_index));
    }

    // Iterate through all values of the input table.
    // All values, which fulfill the filter criteria, will be added to the output table;

    for (ChunkID chunk_id{0}; chunk_id < _input_table->chunk_count(); ++chunk_id) {
      const auto segment_to_scan = _input_table->get_chunk(chunk_id).get_segment(_column_id);

      const std::shared_ptr<ReferenceSegment> ref_segment = std::dynamic_pointer_cast<ReferenceSegment>(segment_to_scan);

      if(ref_segment == nullptr) {
        // non ref segment
        const std::shared_ptr<PosList> pos_list = _get_positions_of_accepted_values(chunk_id, segment_to_scan);

        if (!pos_list->empty()) {
          auto chunk_to_add = _create_chunk(pos_list);
          output_table->emplace_chunk(std::move(chunk_to_add));
        }
      }else{
        // ref semgent
        const std::shared_ptr<PosList> pos_list = _get_positions_of_accepted_values(chunk_id, ref_segment);

        if (!pos_list->empty()) {
          auto chunk_to_add = _create_chunk(pos_list, ref_segment->referenced_table());
          output_table->emplace_chunk(std::move(chunk_to_add));
        }
      }
    }

    if (output_table->row_count() == 0) {
      output_table->emplace_chunk(std::move(_create_chunk(std::make_shared<PosList>())));
    }

    return output_table;
  }

  Chunk _create_chunk(const std::shared_ptr<PosList> pos_list) const {
    Chunk chunk;
    for (ColumnID column_id{0}; column_id < _input_table->column_count(); ++column_id) {
      chunk.add_segment(std::make_shared<ReferenceSegment>(_input_table, column_id, pos_list));
    }
    return chunk;
  }

  Chunk _create_chunk(const std::shared_ptr<PosList> pos_list, const std::shared_ptr<const Table> input_table) const {
    DebugAssert(input_table != nullptr, "Input table is not initialized.");
    Chunk chunk;
    for (ColumnID column_id{0}; column_id < input_table->column_count(); ++column_id) {
      chunk.add_segment(std::make_shared<ReferenceSegment>(input_table, column_id, pos_list));
    }
    return chunk;
  }

  bool _accepted_by_comparison(const T& value) const {
    switch (_scan_type) {
      case ScanType::OpEquals:
        return value == _search_value;
      case ScanType::OpGreaterThan:
        return value > _search_value;
      case ScanType::OpGreaterThanEquals:
        return value >= _search_value;
      case ScanType::OpLessThan:
        return value < _search_value;
      case ScanType::OpLessThanEquals:
        return value <= _search_value;
      case ScanType::OpNotEquals:
        return value != _search_value;
      default:
        throw std::runtime_error("comparison operator is not supported.");
    }
  }

  // gets the positions of accepted values for a value segment of dict. segment
  std::shared_ptr<PosList> _get_positions_of_accepted_values(const ChunkID& chunk_id,
                                                             const std::shared_ptr<BaseSegment>& segment) const{
      std::shared_ptr<PosList> pos_list = std::make_shared<PosList>();
      pos_list->reserve(segment->size() * POS_LIST_RESERVATION_FACTOR);

      // Find out out the type of the segment by using dynamic pointer casts.
      const auto value_segment = std::dynamic_pointer_cast<ValueSegment<T>>(segment);
      if (value_segment != nullptr) {
          return _get_positions_of_accepted_values(chunk_id, value_segment, pos_list);
      }

      const auto dictionary_segment = std::dynamic_pointer_cast<DictionarySegment<T>>(segment);
      if (dictionary_segment != nullptr) {
          return _get_positions_of_accepted_values(chunk_id, dictionary_segment, pos_list);
       }

      throw std::runtime_error("Unsupported segment type.");
  }

  // gets the positions of accepted values for a value segment of dict. segment
  std::shared_ptr<PosList> _get_positions_of_accepted_values(const std::shared_ptr<BaseSegment>& segment,
                                                             std::shared_ptr<PosList>& target_list,
                                                             const std::shared_ptr<PosList>& positions_to_scan) const{
    // Find out out the type of the segment by using dynamic pointer casts.
    const auto value_segment = std::dynamic_pointer_cast<ValueSegment<T>>(segment);
    if (value_segment != nullptr) {
        return _get_positions_of_accepted_values(value_segment, target_list, positions_to_scan);
    }

    const auto dictionary_segment = std::dynamic_pointer_cast<DictionarySegment<T>>(segment);
    if (dictionary_segment != nullptr) {
      return _get_positions_of_accepted_values(dictionary_segment, target_list, positions_to_scan);
    }

    throw std::runtime_error("Unsupported segment type.");
  }

  std::shared_ptr<PosList> _get_positions_of_accepted_values(const ChunkID& chunkID,
                                                             const std::shared_ptr<ValueSegment<T>>& segment,
                                                             std::shared_ptr<PosList> target_list) const {
    auto offset = ChunkOffset{0};
    for (const auto& value : segment->values()) {
      if (_accepted_by_comparison(value)) {
        target_list->emplace_back(RowID{chunkID, offset});
      }
      ++offset;
    }

    return target_list;
  }

    std::shared_ptr<PosList> _get_positions_of_accepted_values(const std::shared_ptr<ValueSegment<T>>& segment,
                                                               std::shared_ptr<PosList> target_list,
                                                               const std::shared_ptr<PosList> positions_to_scan) const {
    const auto& values = segment->values();
    for(const auto& row_id : *positions_to_scan){
      const auto& value = values[row_id.chunk_offset];
        if(_accepted_by_comparison(value)){
          target_list->emplace_back(row_id);
        }
      }

      return target_list;
    }

  std::shared_ptr<PosList> _get_positions_of_accepted_values(const ChunkID& chunkID,
          const std::shared_ptr<DictionarySegment<T>>& segment,
          std::shared_ptr<PosList> target_list) const {

    const auto& attribute_vector = segment->attribute_vector();

    for (size_t att_vec_offset = 0; att_vec_offset < attribute_vector->size(); ++att_vec_offset) {
      const auto& value = segment->value_by_value_id(attribute_vector->get(att_vec_offset));
      if (_accepted_by_comparison(value)) {
        target_list->emplace_back(RowID{chunkID, ChunkOffset(att_vec_offset)});
      }
    }

    return target_list;
  }

    std::shared_ptr<PosList> _get_positions_of_accepted_values(const std::shared_ptr<DictionarySegment<T>>& segment,
                                                               std::shared_ptr<PosList> target_list,
                                                               const std::shared_ptr<PosList> positions_to_scan) const {
      const auto& attribute_vector = segment->attribute_vector();

      for(const auto& row_id : *positions_to_scan){
        const auto& value = segment->value_by_value_id(attribute_vector->get(row_id.chunk_offset));
        if (_accepted_by_comparison(value)) {
          target_list->emplace_back(row_id);
        }
      }

      return target_list;
    }

  std::shared_ptr<PosList> _get_positions_of_accepted_values(const ChunkID& chunkID,
                                                             const std::shared_ptr<ReferenceSegment>& ref_segment) const {
    std::shared_ptr<PosList> pos_list = std::make_shared<PosList>();
    pos_list->reserve(ref_segment->size() * POS_LIST_RESERVATION_FACTOR);
    // list of PosLists, each pos list refers to only one chunk
    const auto& single_chunk_pos_lists = _get_single_chunk_pos_lists(ref_segment->pos_list());

    // iterate through all the single chunk pos lists
    const auto& segment_to_scan = ref_segment->referenced_table()->get_chunk(chunkID).get_segment(_column_id);
    for(const auto& positions_to_scan : *single_chunk_pos_lists){
      pos_list = _get_positions_of_accepted_values(segment_to_scan, pos_list, positions_to_scan);
    }

    return pos_list;
  }

  // this method takes a PosList and creates a list of PosLists so that every PosList is just referenced to a single chunk.
  const std::shared_ptr<std::vector<std::shared_ptr<PosList>>> _get_single_chunk_pos_lists(const std::shared_ptr<const PosList>& pos_list) const {
    std::shared_ptr<std::vector<std::shared_ptr<PosList>>> single_chunk_pos_lists = std::make_shared<std::vector<std::shared_ptr<PosList>>>();
    std::map<ChunkID, uint32_t> chunk_id_to_list_index;

    for(const auto& row_id : *pos_list){
      // Chunk with id of currently iterated row id was previously not processed in this loop.
      const auto pos_lists_index_iter = chunk_id_to_list_index.find(row_id.chunk_id);

      if(pos_lists_index_iter == chunk_id_to_list_index.cend()){
        // a PosList for the ChunkId of currently iterated RowId exits in the list of PosLists
        auto new_pos_list = std::make_shared<PosList>();
        new_pos_list->emplace_back(row_id);
        single_chunk_pos_lists->emplace_back(new_pos_list);
        chunk_id_to_list_index[row_id.chunk_id] = single_chunk_pos_lists->size()-1;
      } else{
        // a PosList for the ChunkId of currently iterated RowId exits in the list of PosLists
        single_chunk_pos_lists->at(pos_lists_index_iter->second)->emplace_back(row_id);
      }
    }
    return single_chunk_pos_lists;
  }

};

}  // namespace opossum
