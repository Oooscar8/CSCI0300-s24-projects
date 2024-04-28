#include <string>

#include "server_test.hpp"
#include "test_utils/test_utils.hpp"

// for simplicity
using namespace std;

constexpr size_t N_SERVERS = 5;
constexpr chrono::milliseconds sleepTime(1000);
static constexpr std::size_t kRandStringLength = 5;
static constexpr std::size_t kNumKeyValPairs = 100;

void ServerTest::test_process_config(shared_ptr<Shardcontroller> sm,
                                     shared_ptr<KvServer> source,
                                     shared_ptr<KvServer> dest,
                                     string dest_addr, Shard shard,
                                     vector<string> keys,
                                     vector<string> values) {
  // move shard to destination server
  ASSERT(test_move(sm, dest_addr, {shard}));
  bool success = false;
  // invoke process_config on both servers until it succeeds
  while (!success) {
    success = dest->process_config() && source->process_config();
  }

  // Requesting pairs from source server should fail
  // because the source server should have updated its config to
  // reflect the fact that it's no longer responsible for these keys
  {
    MultiGetRequest get_req = {keys};
    Response get_res = source->process_request(get_req);
    MultiGetResponse* res = get_if<MultiGetResponse>(&get_res);
    ASSERT(!res);
  }

  // Check that the source server actually deleted the key-value pairs
  // from its store -- getting the keys should fail.
  {
    GetResponse get_res;
    for (std::string key : keys) {
      GetRequest get_req = {key};
      bool res = source->store->Get(&get_req, &get_res);
      ASSERT(!res);
    }
  }

  // Getting keys from destination server should succeed
  // because it should recognize it is now responsible for this shard and
  // received them from the source server
  {
    MultiGetRequest get_req = {keys};
    Response get_res = dest->process_request(get_req);
    MultiGetResponse* res = get_if<MultiGetResponse>(&get_res);
    if (!res) {
      ErrorResponse* err = get_if<ErrorResponse>(&get_res);
      cout_color(RED, err->msg);
    }
    ASSERT(res);
    ASSERT_EQ_VECS(res->values, values);
  }
}

int ServerTest::run_test() {
  string sm_addr = get_host_address("8080");
  shared_ptr<Shardcontroller> sm = start_shardcontroller(sm_addr);

  vector<string> server_addresses = make_server_addresses(N_SERVERS);

  vector<Shard> shards = split_into(N_SERVERS);
  vector<shared_ptr<KvServer>> servers;

  // Start N_SERVERS & move shards to them
  for (size_t i = 0; i < N_SERVERS; i++) {
    shared_ptr<KvServer> ptr =
        start_server<KvServer, const std::string&, const std::string&,
                     uint64_t>(server_addresses[i], sm_addr, 5);
    servers.push_back(ptr);
    ASSERT(test_move(sm, server_addresses[i], vector<Shard>{shards[i]}));
  }

  // At this point, shards should look like this: [0, 7] [8, E] [F, L] [M, S]
  // [T-Z]

  // Sleep to allow the config to update before issuing requests
  this_thread::sleep_for(sleepTime);

  vector<vector<string>> keys;
  vector<vector<string>> vals;
  vector<string> chars_in_shard = {"01234567", "89ABCDE", "FGHIJKL", "MNOPQRS",
                                   "TUWXYZ"};

  // Put some key/value pairs on the servers
  for (size_t i = 0; i < N_SERVERS; i++) {
    // Avoid need for client MultiPut request by only generating keys comprised
    // of chars_in_shard[i]
    keys.push_back(
        make_rand_strs(kNumKeyValPairs, kRandStringLength, chars_in_shard[i]));
    vals.push_back(make_rand_strs(kNumKeyValPairs, kRandStringLength));
    MultiPutRequest req = {keys[i], vals[i]};
    Response put_res = servers[i]->process_request(req);
    MultiPutResponse* res = get_if<MultiPutResponse>(&put_res);
    ASSERT(res);
  }

  this_thread::sleep_for(sleepTime);

  // Getting those keys should succeed
  for (size_t i = 0; i < N_SERVERS; i++) {
    MultiGetRequest req = {keys[i]};
    Response get_res = servers[i]->process_request(req);
    MultiGetResponse* res = get_if<MultiGetResponse>(&get_res);
    ASSERT(res);
    ASSERT_EQ_VECS(res->values, vals[i]);
  }

  // actually test process_config!
  this->test_process_config(sm, servers[0], servers[2], server_addresses[2],
                            {"0", "7"}, keys[0], vals[0]);
  this->test_process_config(sm, servers[1], servers[3], server_addresses[3],
                            {"8", "E"}, keys[1], vals[1]);
  this->test_process_config(sm, servers[3], servers[1], server_addresses[1],
                            {"M", "S"}, keys[3], vals[3]);

  for (shared_ptr<KvServer> server : servers) {
    server->stop();
  }

  sm->stop();

  cout_color(GREEN, "Test passed!");
  return 0;
}

int main() {
  ServerTest test;
  return test.run_test();
}
