#include <cassert>
#include <map>
#include <string>

#include "common/shard.hpp"
#include "net/network_helpers.hpp"
#include "server/server.hpp"
#include "test_utils/test_utils.hpp"

// for simplicity
using namespace std;

constexpr size_t N_SERVERS = 20;

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
    // Joining a server that's already joined should fail
    ASSERT(test_join(sm, server_addresses[i], false));
  }

  cout_color(GREEN, "Test passed!");

  sm->stop();

  return 0;
}
