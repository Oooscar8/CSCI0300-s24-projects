#ifndef SHARDKVCLIENT_MOVECOMMAND_HPP
#define SHARDKVCLIENT_MOVECOMMAND_HPP

#include <algorithm>
#include <iomanip>
#include <memory>
#include <sstream>

#include "client/shardkv_client.hpp"
#include "common/shard.hpp"
#include "common/utils.hpp"
#include "repl/replcommand.hpp"

class MoveCommand : public ReplCommand {
 public:
  explicit MoveCommand(std::shared_ptr<ShardKvClient> c) : client(c) {
  }

  void handle(const std::string& s) override;

  std::string name() const override;
  std::string params() const override;
  std::string description() const override;

 private:
  std::shared_ptr<ShardKvClient> client;
};

#endif /* end of include guard */
