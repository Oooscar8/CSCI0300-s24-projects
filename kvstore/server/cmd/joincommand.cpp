#include "joincommand.hpp"

void JoinCommand::handle(const std::string&) {
  auto res = this->server->Join();
  if (!res) {
    cerr_color(RED, "Failed to send Join request to shardcontroller.");
    return;
  }
}

std::string JoinCommand::name() const {
  return "join";
}

std::string JoinCommand::params() const {
  return "";
}

std::string JoinCommand::description() const {
  return "tells the Shardcontroller that this key-value server is available "
         "for shard distribution.";
}
