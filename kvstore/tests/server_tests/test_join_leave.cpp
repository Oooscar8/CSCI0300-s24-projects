#include <string>

#include "common/config.hpp"
#include "common/shard.hpp"
#include "server/server.hpp"
#include "server_test.hpp"
#include "test_utils/test_utils.hpp"

// for simplicity
using namespace std;

constexpr size_t N_SERVERS = 5;

int main() {
  string sm_addr = get_host_address("8080");
  shared_ptr<Shardcontroller> sm = start_shardcontroller(sm_addr);

  vector<string> server_addresses = make_server_addresses(N_SERVERS);

  vector<Shard> shards = split_into(N_SERVERS);

  // Start N_SERVERS & move shards to them
  for (size_t i = 0; i < N_SERVERS; i++) {
    shared_ptr<KvServer> server =
        start_server<KvServer, const std::string&, const std::string&,
                     uint64_t>(server_addresses[i], sm_addr, 5);
    // Servers should join on start, so subsequent join request should fail
    ASSERT(test_join(sm, server_addresses[i], false));

    server->stop();
    // Servers should leave on stop, so subsequent leave request should fail
    ASSERT(test_leave(sm, server_addresses[i], false));
  }

  sm->stop();

  cout_color(GREEN, "Test passed!");
  return 0;
}