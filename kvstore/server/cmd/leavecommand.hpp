#ifndef SERVER_LEAVECOMMAND_HPP
#define SERVER_LEAVECOMMAND_HPP

#include <algorithm>
#include <memory>
#include <sstream>

#include "../server.hpp"
#include "common/utils.hpp"
#include "repl/replcommand.hpp"

class LeaveCommand : public ReplCommand {
 public:
  explicit LeaveCommand(std::shared_ptr<KvServer> s) : server(s) {
  }

  void handle(const std::string& s) override;

  std::string name() const override;
  std::string params() const override;
  std::string description() const override;

 private:
  std::shared_ptr<KvServer> server;
};

#endif /* end of include guard */
