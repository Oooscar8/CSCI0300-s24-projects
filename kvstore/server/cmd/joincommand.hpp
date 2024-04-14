#ifndef SERVER_JOINCOMMAND_HPP
#define SERVER_JOINCOMMAND_HPP

#include <algorithm>
#include <memory>
#include <sstream>

#include "../server.hpp"
#include "common/utils.hpp"
#include "repl/replcommand.hpp"

class JoinCommand : public ReplCommand {
 public:
  explicit JoinCommand(std::shared_ptr<KvServer> s) : server(s) {
  }

  void handle(const std::string&) override;

  std::string name() const override;
  std::string params() const override;
  std::string description() const override;

 private:
  std::shared_ptr<KvServer> server;
};

#endif /* end of include guard */
