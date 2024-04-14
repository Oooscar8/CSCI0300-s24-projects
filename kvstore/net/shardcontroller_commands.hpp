#ifndef NET_SHARDCONTROLLER_COMMANDS_HPP
#define NET_SHARDCONTROLLER_COMMANDS_HPP

#include <optional>
#include <string>
#include <vector>

#include "common/config.hpp"
#include "common/shard.hpp"

// Requests
struct JoinRequest {
  std::string server;
};
struct LeaveRequest {
  std::string server;
};
struct MoveRequest {
  std::string server;
  std::vector<Shard> shards;
};
struct QueryRequest {};

// Responses
struct JoinResponse {};
struct LeaveResponse {};
struct MoveResponse {};
struct QueryResponse {
  ShardControllerConfig config;
};

#endif /* end of include guard */
