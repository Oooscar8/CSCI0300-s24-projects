#include "querycommand.hpp"

void ClientQueryCommand::handle(const std::string&) {
  auto config = this->client->Query();
  if (!config) {
    cerr_color(RED, "Failed to query shardcontroller.");
    return;
  }

  std::cout << config->print();
}

std::string ClientQueryCommand::name() const {
  return "query";
}

std::string ClientQueryCommand::params() const {
  return "";
}
std::string ClientQueryCommand::description() const {
  return "Gets the current shardcontroller configuration.";
}
