#include <string>

#include "client/shardkv_client.hpp"
#include "common/config.hpp"
#include "common/shard.hpp"
#include "server/server.hpp"
#include "test_utils/test_utils.hpp"

// for simplicity
using namespace std;

constexpr size_t N_SERVERS = 5;
static constexpr std::size_t kRandStringLength = 5;
static constexpr std::size_t kNumKeyValPairs = 20;
static constexpr std::size_t kNumBatches = 5;

int main() {
  string sm_addr = get_host_address("8080");
  shared_ptr<Shardcontroller> sm = start_shardcontroller(sm_addr);

  shared_ptr<ShardKvClient> client = make_shared<ShardKvClient>(sm_addr);

  vector<string> server_addresses = make_server_addresses(N_SERVERS);

  vector<Shard> shards = split_into(N_SERVERS);
  vector<shared_ptr<KvServer>> servers;

  // Start N_SERVERS - 1 & move shards to them
  for (size_t i = 0; i < N_SERVERS - 1; i++) {
    shared_ptr<KvServer> ptr =
        start_server<KvServer, const std::string&, const std::string&,
                     uint64_t>(server_addresses[i], sm_addr, 5);
    servers.push_back(ptr);
    JoinRequest rq{server_addresses[i]};
    sm->Join(&rq, {});
    ASSERT(test_move(sm, server_addresses[i], vector<Shard>{shards[i]}));
  }

  // At this point, shards should look like this: [0, 7] [8, E] [F, L] [M, S]

  vector<vector<string>> keys;
  vector<vector<string>> vals;

  // Generate kNumKeyValPairs random strings for each of the kNumBatches "batch"
  // of kv pairs
  for (size_t i = 0; i < kNumBatches; i++) {
    // don't put T-Z in keys so that no key starts with those letters -- no
    // server has shard T-Z
    keys.push_back(
        make_rand_strs(kNumKeyValPairs, kRandStringLength,
                       "0123456789ABCDEFGHIJKLMNOPQRSabcdefghijklmnopqrs"));
    vals.push_back(make_rand_strs(kNumKeyValPairs, kRandStringLength));
  }

  // Sleep to allow the config to update before issuing requests
  this_thread::sleep_for(500ms);

  // Put key-value pairs on the servers
  // (Avoid using MultiPut so that we only test MultiGet functionality here)
  for (size_t i = 0; i < kNumBatches; i++) {
    for (size_t j = 0; j < kNumKeyValPairs; j++) {
      ASSERT(client->Put(keys[i][j], vals[i][j]));
    }
  }

  const auto multiget_start{chrono::steady_clock::now()};
  // issue MultiGet request for each batch of random keys; check for success &
  // correctness
  for (size_t i = 0; i < kNumBatches; i++) {
    optional<vector<string>> return_val = client->MultiGet(keys[i]);
    ASSERT(return_val);
    ASSERT_EQ_VECS(*return_val, vals[i]);
  }
  const auto multiget_end{chrono::steady_clock::now()};

  const auto get_start{chrono::steady_clock::now()};
  // issue Get request for each random key; check for success & correctness
  for (size_t i = 0; i < kNumBatches; i++) {
    for (size_t j = 0; j < kNumKeyValPairs; j++) {
      optional<string> return_val = client->Get(keys[i][j]);
      ASSERT(return_val);
      ASSERT_EQ(*return_val, vals[i][j]);
    }
  }
  const auto get_end{chrono::steady_clock::now()};

  const chrono::duration<double> multiget_elapsed{multiget_end -
                                                  multiget_start};
  const chrono::duration<double> get_elapsed{get_end - get_start};
  // MultiGet performance should be better than just naively issuing Gets for
  // every key
  ASSERT(multiget_elapsed < get_elapsed);

  // Since no server has shard T-Z, this MultiGet should fail
  vector<string> invalid_keys =
      make_rand_strs(kNumKeyValPairs, kRandStringLength, "TUVWXYZZtuvwxyz");
  optional<vector<string>> ptr = client->MultiGet({invalid_keys});
  ASSERT(!ptr);

  for (shared_ptr<KvServer> server : servers) {
    server->stop();
  }

  sm->stop();

  cout_color(GREEN, "Test passed!");
  return 0;
}
