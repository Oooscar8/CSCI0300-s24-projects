#ifndef SHARDKV_CLIENT_HPP
#define SHARDKV_CLIENT_HPP

#include <array>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "client.hpp"
#include "common/config.hpp"
#include "net/network_conn.hpp"
#include "net/network_messages.hpp"
#include "simple_client.hpp"

class ShardKvClient : public Client {
 public:
  explicit ShardKvClient(const std::string& sm_addr)
      : shardcontroller_addr(sm_addr) {
    this->shardcontroller_conn = connect_to_server(this->shardcontroller_addr);
    if (!this->shardcontroller_conn) {
      cerr_color(RED, "Failed to connect to shardcontroller at ",
                 this->shardcontroller_addr, '.');
      exit(EXIT_FAILURE);
    }
    cout_color(BLUE, "Connected to shardcontroller at ",
               this->shardcontroller_addr, '.');
  }

  ~ShardKvClient() {
    this->shardcontroller_conn->shutdown();
  }

  // ShardKvStore functions
  std::optional<std::string> Get(const std::string& key);

  bool Put(const std::string& key, const std::string& value);

  bool Append(const std::string& key, const std::string& value);

  std::optional<std::string> Delete(const std::string& key);

  std::optional<std::vector<std::string>> MultiGet(
      const std::vector<std::string>& keys);

  bool MultiPut(const std::vector<std::string>& keys,
                const std::vector<std::string>& values);

  bool GDPRDelete(const std::string& user) {
    assert(false);
  }

  // Shardcontroller functions
  std::optional<ShardControllerConfig> Query();
  bool Move(const std::string& dest_server, const std::vector<Shard>& shards);

 private:
  std::string shardcontroller_addr;
  std::shared_ptr<ServerConn> shardcontroller_conn;
};

#endif /* end of include guard */
