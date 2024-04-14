#ifndef SHARDKVCLIENT_ClientQueryCommand_HPP
#define SHARDKVCLIENT_ClientQueryCommand_HPP

#include <memory>
#include <sstream>

#include "client/shardkv_client.hpp"
#include "repl/replcommand.hpp"

class ClientQueryCommand : public ReplCommand {
 public:
  explicit ClientQueryCommand(std::shared_ptr<ShardKvClient> c) : client(c) {
  }

  void handle(const std::string&) override;

  std::string name() const override;
  std::string params() const override;
  std::string description() const override;

 private:
  std::shared_ptr<ShardKvClient> client;
};

#endif /* end of include guard */
