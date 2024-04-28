#include <cassert>
#include <map>
#include <string>

#include "common/shard.hpp"
#include "net/network_helpers.hpp"
#include "server/server.hpp"
#include "test_utils/test_utils.hpp"

// for simplicity
using namespace std;

constexpr size_t N_SERVERS = 5;

int main() {
  string sm_addr = get_host_address("8080");
  shared_ptr<Shardcontroller> sm = start_shardcontroller(sm_addr);

  vector<string> server_addresses = make_server_addresses(N_SERVERS);
  // correct_config represents what the configuration should be at any given
  // point we assert equality between correct_config and the configuration your
  // shardcontroller.Query() returns
  map<string, vector<Shard>> correct_config;

  // Join N_SERVERS and assert that they're in the config after joining
  for (size_t i = 0; i < N_SERVERS; i++) {
    ASSERT(test_join(sm, server_addresses[i], true));
    correct_config[server_addresses[i]] = vector<Shard>{};
    ASSERT_EQ_CONFIGS(query_config(sm), correct_config);
  }

  vector<Shard> shards = split_into(N_SERVERS);
  for (size_t i = 0; i < N_SERVERS; i++) {
    ASSERT(test_move(sm, server_addresses[i], vector<Shard>{shards[i]}));
    correct_config[server_addresses[i]] = vector<Shard>{shards[i]};
  }

  // At this point, shards should look like this: [0, 7] [8, E] [F, L] [M, S],
  // [T, Z]

  // Moving to a nonexistent server fails
  ASSERT(!test_move(sm, "nonexistent:123", {{"1", "7"}}));

  // Move [0, 7] to server_addresses[1]
  // (completely_contained for server_addresses[0], no_overlap for
  // server_addresses[1]):
  ASSERT(test_move(sm, server_addresses[1], {{"0", "7"}}));
  correct_config[server_addresses[0]] = vector<Shard>{};
  correct_config[server_addresses[1]] = {{"0", "7"}, {"8", "E"}};
  ASSERT_EQ_CONFIGS(query_config(sm), correct_config);

  // Move [A, G] to server_addresses[3]: (overlap_end for server_addresses[1],
  // overlap_start for server_addresses[2], no_overlap for server_addresses[3])
  ASSERT(test_move(sm, server_addresses[3], {{"A", "G"}}));
  correct_config[server_addresses[1]] = {{"0", "7"}, {"8", "9"}};
  correct_config[server_addresses[2]] = {{"H", "L"}};
  correct_config[server_addresses[3]] = {{"A", "G"}, {"M", "S"}};
  ASSERT_EQ_CONFIGS(query_config(sm), correct_config);

  // Move [0, 7] to server_addresses[1] (completely_contained for
  // server_addresses[1]): since server_addresses[1] already has this shard, the
  // config shouldn't change
  ASSERT(test_move(sm, server_addresses[1], {{"0", "7"}}));
  ASSERT_EQ_CONFIGS(query_config(sm), correct_config);

  // Move [I, K] to server_addresses[2] (completely_contains for
  // server_addresses[2]):
  ASSERT(test_move(sm, server_addresses[2], {{"I", "K"}}));
  correct_config[server_addresses[2]] = {{"H", "H"}, {"I", "K"}, {"L", "L"}};
  ASSERT_EQ_CONFIGS(query_config(sm), correct_config);

  // Move [H, H], [L, L] to server_addresses[0]
  // (no_overlap for server_addresses[0], completely_contained for
  // server_addresses[2] (twice)):
  ASSERT(test_move(sm, server_addresses[0], {{"H", "H"}, {"L", "L"}}));
  correct_config[server_addresses[2]] = {{"I", "K"}};
  correct_config[server_addresses[0]] = {{"H", "H"}, {"L", "L"}};
  ASSERT_EQ_CONFIGS(query_config(sm), correct_config);

  // Move [1, Y] to server_addresses[4]:
  // (completely_contained for server_addresses[0, 2, 3], overlap_end for
  // server_addresses[1], overlap_start for server_addresses[4])
  ASSERT(test_move(sm, server_addresses[4], {{"1", "Y"}}));
  correct_config[server_addresses[0]] = vector<Shard>{};
  correct_config[server_addresses[1]] = {{"0", "0"}};
  correct_config[server_addresses[2]] = vector<Shard>{};
  correct_config[server_addresses[3]] = vector<Shard>{};
  correct_config[server_addresses[4]] = {{"1", "Y"}, {"Z", "Z"}};
  ASSERT_EQ_CONFIGS(query_config(sm), correct_config);

  cout_color(GREEN, "Test passed!");

  sm->stop();

  return 0;
}
