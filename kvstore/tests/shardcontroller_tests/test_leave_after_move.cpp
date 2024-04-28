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
  /*
  NOTE: This test performs the same types of moves as test_complex_moves.
  It builds on that test by having some servers leave between those moves.
  If you pass test_complex_moves and fail this test, you have a problem in your
  Leave() shard reassignment logic.
  */
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

  // Move [0, 7] to server_addresses[1] (completely contained):
  ASSERT(test_move(sm, server_addresses[1], {{"0", "7"}}));
  correct_config[server_addresses[0]] = vector<Shard>{};
  correct_config[server_addresses[1]] = {{"0", "7"}, {"8", "E"}};
  ASSERT_EQ_CONFIGS(query_config(sm), correct_config);

  // Leave server_addresses[1]; server[1]'s shards should be moved to server[0]
  ASSERT(test_leave(sm, server_addresses[1], true));
  correct_config.erase(server_addresses[1]);
  correct_config[server_addresses[0]] = {{"0", "7"}};

  // Move [A, G] to server_addresses[3] (spans multiple servers):
  ASSERT(test_move(sm, server_addresses[3], {{"A", "G"}}));
  correct_config[server_addresses[0]] = {{"0", "7"}, {"8", "9"}};
  correct_config[server_addresses[2]] = {{"H", "L"}};
  correct_config[server_addresses[3]] = {{"A", "G"}, {"M", "S"}};
  ASSERT_EQ_CONFIGS(query_config(sm), correct_config);

  // Move [0, 7] to server_addresses[0] (nothing should happen):
  ASSERT(test_move(sm, server_addresses[0], {{"0", "7"}}));
  ASSERT_EQ_CONFIGS(query_config(sm), correct_config);

  // Move [I, K] to server_addresses[2] (this will split server_addresses[2]'s
  // shards):
  ASSERT(test_move(sm, server_addresses[2], {{"I", "K"}}));
  correct_config[server_addresses[2]] = {{"H", "H"}, {"I", "K"}, {"L", "L"}};
  ASSERT_EQ_CONFIGS(query_config(sm), correct_config);

  // Leave server_addresses[2]; server[2]'s shards should be moved to server[0]
  ASSERT(test_leave(sm, server_addresses[2], true));
  correct_config.erase(server_addresses[2]);
  correct_config[server_addresses[0]] = {
      {"0", "7"}, {"8", "9"}, {"H", "H"}, {"I", "K"}, {"L", "L"}};

  // Moving to server_addresses[1] should fail b/c it's already left
  ASSERT(!test_move(sm, server_addresses[1], {{"H", "H"}, {"L", "L"}}));

  // Move [H, H], [L, L] to server_addresses[3] (test moving multiple shards):
  ASSERT(test_move(sm, server_addresses[3], {{"H", "H"}, {"L", "L"}}));
  correct_config[server_addresses[0]] = {{"0", "7"}, {"8", "9"}, {"I", "K"}};
  correct_config[server_addresses[3]] = {
      {"A", "G"}, {"H", "H"}, {"L", "L"}, {"M", "S"}};
  ASSERT_EQ_CONFIGS(query_config(sm), correct_config);

  // Move [1, Y] to server_addresses[4]:
  ASSERT(test_move(sm, server_addresses[4], {{"1", "Y"}}));
  correct_config[server_addresses[0]] = {{"0", "0"}};
  correct_config[server_addresses[3]] = vector<Shard>{};
  correct_config[server_addresses[4]] = {{"1", "Y"}, {"Z", "Z"}};
  ASSERT_EQ_CONFIGS(query_config(sm), correct_config);

  // Leave server_addresses[0]; server[0]'s shards should be moved to server[3]
  ASSERT(test_leave(sm, server_addresses[0], true));
  correct_config.erase(server_addresses[0]);
  correct_config[server_addresses[3]] = {{"0", "0"}};
  ASSERT_EQ_CONFIGS(query_config(sm), correct_config);

  // Leave server_addresses[3]; server[3]'s shards should be moved to server[4]
  ASSERT(test_leave(sm, server_addresses[3], true));
  correct_config.erase(server_addresses[3]);
  correct_config[server_addresses[4]] = {{"0", "0"}, {"1", "Y"}, {"Z", "Z"}};
  ASSERT_EQ_CONFIGS(query_config(sm), correct_config);

  // Leave server_addresses[4]; config should be empty
  ASSERT(test_leave(sm, server_addresses[4], true));
  correct_config.erase(server_addresses[4]);
  ASSERT_EQ_CONFIGS(query_config(sm), correct_config);

  cout_color(GREEN, "Test passed!");

  sm->stop();

  return 0;
}
