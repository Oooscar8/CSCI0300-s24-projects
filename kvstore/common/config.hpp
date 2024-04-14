#ifndef COMMON_CONFIG_HPP
#define COMMON_CONFIG_HPP

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "common/shard.hpp"

// Struct representing a Shardcontroller's configuration.
struct ShardControllerConfig {
  // map each server address to the shards it's responsible for
  std::map<std::string, std::vector<Shard>> server_to_shards;

  // Pretty printing of configuration
  std::string print();
  // Gets the server with the shard for the key.
  std::optional<std::string> get_server(const std::string& key);
};

#endif /* end of include guard */
