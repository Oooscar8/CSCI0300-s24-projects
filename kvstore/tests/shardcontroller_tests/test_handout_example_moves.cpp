#include <cassert>
#include <map>
#include <string>

#include "common/shard.hpp"
#include "net/network_helpers.hpp"
#include "server/server.hpp"
#include "test_utils/test_utils.hpp"

// for simplicity
using namespace std;

// NOTE: This is the example from the handout!
// See Example: Configuration Walkthrough in Part 1
// The "TABLE #" annotations correspond to the tables in the example;
// e.g., TABLE 2 means the second table in the example
int main() {
  string sm_addr = get_host_address("8080");
  shared_ptr<Shardcontroller> sm = start_shardcontroller(sm_addr);

  string bear_server = "bear:713";
  string elephant_server = "elephant:4000";
  string tiger_server = "tiger:9999";
  // correct_config represents what the configuration should be at any given
  // point we assert equality between correct_config and the configuration your
  // shardcontroller.Query() returns
  map<string, vector<Shard>> correct_config;

  // TABLE #1
  // Join servers and assert that they're in the config after joining
  ASSERT(test_join(sm, bear_server, true));
  correct_config[bear_server] = vector<Shard>{};
  ASSERT_EQ_CONFIGS(query_config(sm), correct_config);

  ASSERT(test_join(sm, elephant_server, true));
  correct_config[elephant_server] = vector<Shard>{};
  ASSERT_EQ_CONFIGS(query_config(sm), correct_config);

  ASSERT(test_join(sm, tiger_server, true));
  correct_config[tiger_server] = vector<Shard>{};
  ASSERT_EQ_CONFIGS(query_config(sm), correct_config);

  // TABLE #2
  // Initial moves
  // Set up correct_config with shards we're moving
  correct_config[elephant_server] = vector<Shard>{{"0", "9"}};
  correct_config[tiger_server] = vector<Shard>{{"A", "G"}};
  correct_config[bear_server] = vector<Shard>{{"H", "L"}};
  // Execute moves with shards in correct_config; assert moves succeed
  ASSERT(test_move(sm, elephant_server, correct_config[elephant_server]));
  ASSERT(test_move(sm, tiger_server, correct_config[tiger_server]));
  ASSERT(test_move(sm, bear_server, correct_config[bear_server]));
  // Assert the shardcontroller config after moves equals correct_config
  ASSERT_EQ_CONFIGS(query_config(sm), correct_config);

  // TABLE #3
  // bear:713 leaves; its shards should be reassigned to elephant:4000
  ASSERT(test_leave(sm, bear_server, true));
  correct_config[elephant_server] = vector<Shard>{{"0", "9"}, {"H", "L"}};
  correct_config.erase(bear_server);
  ASSERT_EQ_CONFIGS(query_config(sm), correct_config);

  // TABLE #4
  // Move [2, 5] to tiger:9999
  correct_config[elephant_server] =
      vector<Shard>{{"0", "1"}, {"6", "9"}, {"H", "L"}};
  correct_config[tiger_server] = vector<Shard>{{"2", "5"}, {"A", "G"}};
  ASSERT(test_move(sm, tiger_server, vector<Shard>{{"2", "5"}}));
  ASSERT_EQ_CONFIGS(query_config(sm), correct_config);

  // TABLE #5
  // bear:713 rejoins; move [A, Z] to it
  ASSERT(test_join(sm, bear_server, true));
  correct_config[elephant_server] = vector<Shard>{{"0", "1"}, {"6", "9"}};
  correct_config[tiger_server] = vector<Shard>{{"2", "5"}};
  correct_config[bear_server] = vector<Shard>{{"A", "Z"}};
  ASSERT(test_move(sm, bear_server, vector<Shard>{{"A", "Z"}}));
  ASSERT_EQ_CONFIGS(query_config(sm), correct_config);

  cout_color(GREEN, "Test passed!");

  sm->stop();

  return 0;
}
