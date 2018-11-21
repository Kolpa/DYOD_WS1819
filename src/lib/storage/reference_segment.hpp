#include "base_segment.hpp"
#include "table.hpp"

namespace opossum {
// ReferenceSegment is a specific column type that stores all its values as position list of a referenced column
class ReferenceSegment : public BaseSegment {
 public:
  // creates a reference column
  // the parameters specify the positions and the referenced column
  ReferenceSegment(const std::shared_ptr<const Table> referenced_table, const ColumnID referenced_column_id,
                   const std::shared_ptr<const PosList> pos);
  const AllTypeVariant operator[](const size_t i) const override;
  void append(const AllTypeVariant&) override { throw std::logic_error("ReferenceSegment is immutable"); };
  size_t size() const override;
  const std::shared_ptr<const PosList> pos_list() const;
  const std::shared_ptr<const Table> referenced_table() const;
  ColumnID referenced_column_id() const;
};
}  // namespace opossum