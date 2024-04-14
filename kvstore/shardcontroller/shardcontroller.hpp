#ifndef SHARDCONTROLLER_HPP
#define SHARDCONTROLLER_HPP

#include <string>
#include <vector>

#include "common/shard.hpp"
#include "net/shardcontroller_commands.hpp"

class Shardcontroller {
 public:
  virtual ~Shardcontroller() = default;

  virtual bool Join(const JoinRequest* req, JoinResponse* res) = 0;
  virtual bool Leave(const LeaveRequest* req, LeaveResponse* res) = 0;
  virtual bool Move(const MoveRequest* req, MoveResponse* res) = 0;
  virtual bool Query(const QueryRequest* req, QueryResponse* res) = 0;

  virtual int start() = 0;
  virtual void stop() = 0;
};

#endif /* end of include guard */
