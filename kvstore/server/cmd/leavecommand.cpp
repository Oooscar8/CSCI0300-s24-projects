#include "leavecommand.hpp"

void LeaveCommand::handle(const std::string&) {
  auto res = this->server->Leave();
  if (!res) {
    cerr_color(RED, "Failed to send Leave request to shardcontroller.");
    return;
  }
}

std::string LeaveCommand::name() const {
  return "leave";
}

std::string LeaveCommand::params() const {
  return "";
}

std::string LeaveCommand::description() const {
  return "tells the Shardcontroller that this key-value server is no longer "
         "available for shard distribution.";
}
