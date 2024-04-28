#include <algorithm>
#include <cassert>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "common/shard.hpp"
#include "net/network_helpers.hpp"
#include "server/server.hpp"
#include "test_utils/test_utils.hpp"

// for simplicity
using namespace std;

constexpr size_t N_SERVERS = 1000;
constexpr size_t N_CHUNKS = 10;

int main() {
  string sm_addr = get_host_address("8080");

  shared_ptr<Shardcontroller> sm = start_shardcontroller(sm_addr);

  // make a vector of length N_CHUNKS, where each element is a vector of
  // N_SERVERS / N_CHUNKS servers
  vector<vector<string>> server_chunks =
      make_server_chunks(N_SERVERS, N_CHUNKS);
  // correct_config represents what the configuration should be at any given
  // point we assert equality between correct_config and the configuration your
  // shardcontroller.Query() returns
  map<string, vector<Shard>> correct_config;

  // Join N_SERVERS for each chunk
  execute_in_parallel(
      [&](vector<string>&& chunk) {
        for (string server : chunk) {
          ASSERT(test_join(sm, server, true));
        }
      },
      server_chunks);

  for (vector<string> chunk : server_chunks) {
    for (string server : chunk) {
      correct_config[server] = vector<Shard>{};
    }
  }

  // Assert that all the servers joined
  ASSERT_EQ_CONFIGS(query_config(sm), correct_config);

  /* Shard the keyspace into N_SERVERS / N_CHUNKS shards, then concurrently move
    these shards to N_CHUNKS different servers. For example, with N_SERVERS =
    1000 and N_CHUNKS = 10, we create server_chunks = {<100 servers here>, <next
    100 servers>, .... <10th vector of 100 servers>}, then call split_into to
    make 1000/10 = 100 shards. Then, we concurrently iterate through each
    "chunk" (i.e., vector of 100 servers) and assign each of the 100 shards to
    the server at the corresponding index. In our example, shard #90 is moved to
    the 90th server in each of the 10 chunks concurrently. Since these moves
    happen concurrently, there's no guarantee as to which server gets each shard
    in the end, but if your code is thread-safe, all of the calls to move should
    succeed!
  */
  vector<Shard> shards = split_into(N_SERVERS / N_CHUNKS);
  execute_in_parallel(
      [&](vector<string>&& chunk) {
        for (size_t i = 0; i < chunk.size(); i++) {
          ASSERT(test_move(sm, chunk[i], {shards[i]}));
        }
      },
      server_chunks);

  // Now, we concurrently move all of the shards to a single server.
  // If your code is thread-safe, all of the shards should be assigned to the
  // first server after the moves are finished.
  execute_in_parallel(
      [&](Shard&& shard) {
        ASSERT(test_move(sm, server_chunks[0][0], {shard}));
      },
      shards);

  correct_config[server_chunks[0][0]] = shards;

  // Assert that all the shards are assigned to server_chunks[0][0]
  ASSERT_EQ_CONFIGS(query_config(sm), correct_config);

  cout_color(GREEN, "Test passed!");

  sm->stop();

  return 0;
}
