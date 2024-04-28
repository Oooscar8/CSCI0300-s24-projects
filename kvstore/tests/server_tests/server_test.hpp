#include "common/config.hpp"
#include "common/shard.hpp"
#include "server/server.hpp"
#include "shardcontroller/shardcontroller.hpp"

using namespace std;

class ServerTest {
 public:
  int run_test();
  void test_process_config(shared_ptr<Shardcontroller> sm,
                           shared_ptr<KvServer> source,
                           shared_ptr<KvServer> dest, string dest_addr,
                           Shard shard, vector<string> keys,
                           vector<string> values);
};