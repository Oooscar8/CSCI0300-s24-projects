#ifndef SHARDCONTROLLER_QUERYCOMMAND_HPP
#define SHARDCONTROLLER_QUERYCOMMAND_HPP

#include <memory>
#include <sstream>

#include "../shardcontroller.hpp"
#include "repl/replcommand.hpp"

class QueryCommand : public ReplCommand {
 public:
  explicit QueryCommand(std::shared_ptr<Shardcontroller> sm)
      : shardcontroller(sm) {
  }

  void handle(const std::string&) override;

  std::string name() const override;
  std::string params() const override;
  std::string description() const override;

 private:
  std::shared_ptr<Shardcontroller> shardcontroller;
};

#endif /* end of include guard */
