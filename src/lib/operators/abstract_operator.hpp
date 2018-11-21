#include "types.hpp"
#include "storage/table.hpp"

namespace opossum {

class AbstractOperator : private Noncopyable {
 public:
  AbstractOperator(const std::shared_ptr<const AbstractOperator> left = nullptr,
                   const std::shared_ptr<const AbstractOperator> right = nullptr);
  // we need to explicitly set the move constructor to default when
  // we overwrite the copy constructor
  AbstractOperator(AbstractOperator&&) = default;
  AbstractOperator& operator=(AbstractOperator&&) = default;
  void execute();
  // returns the result of the operator
  std::shared_ptr<const Table> get_output() const;
  // Get the input operators.
  std::shared_ptr<const AbstractOperator> input_left() const;
  std::shared_ptr<const AbstractOperator> input_right() const;

 protected:
  // abstract method to actually execute the operator
  // execute and get_output are split into two methods to allow for easier
  // asynchronous execution
  virtual std::shared_ptr<const Table> _on_execute() = 0;
  std::shared_ptr<const Table> _input_table_left() const;
  std::shared_ptr<const Table> _input_table_right() const;
  // Shared pointers to input operators, can be nullptr.
  std::shared_ptr<const AbstractOperator> _input_left;
  std::shared_ptr<const AbstractOperator> _input_right;
  // Is nullptr until the operator is executed
  std::shared_ptr<const Table> _output;
};
}  // namespace opossum