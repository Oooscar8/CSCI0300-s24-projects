#include "test_utils.hpp"

bool put_range(KvStore& store, const std::vector<std::string>& keys,
               const std::vector<std::string>& vals, std::size_t start,
               std::size_t end) {
  auto put_req = PutRequest{};
  auto put_res = PutResponse{};
  for (auto i = start; i < end; ++i) {
    put_req.key = keys[i];
    put_req.value = vals[i];
    ASSERT(store.Put(&put_req, &put_res));
  }
  return true;
}

bool get_range(KvStore& store, const std::vector<std::string>& keys,
               const std::vector<std::string>& vals, std::size_t start,
               std::size_t end) {
  auto get_req = GetRequest{};
  auto get_res = GetResponse{};
  for (auto i = start; i < end; ++i) {
    get_req.key = keys[i];
    ASSERT(store.Get(&get_req, &get_res));
    if (get_res.value != vals[i]) {
      std::cout << "Expected " << vals[i] << ", got " << get_res.value
                << std::endl;
      ASSERT(get_res.value == vals[i]);
    }
  }
  return true;
}

bool del_range(KvStore& store, const std::vector<std::string>& keys,
               const std::vector<std::string>& vals, std::size_t start,
               std::size_t end) {
  auto del_req = DeleteRequest{};
  auto del_res = DeleteResponse{};
  for (std::size_t i = start; i < end; ++i) {
    del_req.key = keys[i];
    ASSERT(store.Delete(&del_req, &del_res));
    ASSERT(del_res.value == vals[i]);
  }
  return true;
}

bool push_get_delete_range(KvStore& store, const std::vector<std::string>& keys,
                           const std::vector<std::string>& vals,
                           std::size_t start, std::size_t end) {
  return put_range(store, keys, vals, start, end) &&
         get_range(store, keys, vals, start, end) &&
         del_range(store, keys, vals, start, end);
}

bool multiput_range(KvStore& store, const std::vector<std::string>& keys,
                    const std::vector<std::string>& vals, std::size_t start,
                    std::size_t end, std::size_t n_per_mput) {
  std::size_t n_multiputs = (end - start) / n_per_mput;
  for (std::size_t i = 0; i < n_multiputs; i++) {
    auto mput_start = start + i * n_per_mput;
    auto mput_end = mput_start + n_per_mput;
    auto mput_req = MultiPutRequest{
        .keys = std::vector<std::string>(keys.begin() + mput_start,
                                         keys.begin() + mput_end),
        .values = std::vector<std::string>(vals.begin() + mput_start,
                                           vals.begin() + mput_end)};
    auto mput_res = MultiPutResponse{};
    ASSERT(store.MultiPut(&mput_req, &mput_res));
  }
  return true;
}

bool multiget_range(KvStore& store, const std::vector<std::string>& keys,
                    const std::vector<std::string>& vals, std::size_t start,
                    std::size_t end, std::size_t n_per_mget) {
  std::size_t n_multigets = (end - start) / n_per_mget;
  for (std::size_t i = 0; i < n_multigets; i++) {
    auto mget_start = start + i * n_per_mget;
    auto mget_end = mget_start + n_per_mget;
    auto mget_req = MultiGetRequest{
        .keys = std::vector<std::string>(keys.begin() + mget_start,
                                         keys.begin() + mget_end)};
    auto mget_res = MultiGetResponse{};
    ASSERT(store.MultiGet(&mget_req, &mget_res));
    ASSERT_EQ_VECS(std::vector<std::string>(vals.begin() + mget_start,
                                            vals.begin() + mget_end),
                   mget_res.values);
  }
  return true;
}

bool multiput_multiget_range(KvStore& store,
                             const std::vector<std::string>& keys,
                             const std::vector<std::string>& vals,
                             std::size_t start, std::size_t end,
                             std::size_t n_per_mop) {
  return multiput_range(store, keys, vals, start, end, n_per_mop) &&
         multiget_range(store, keys, vals, start, end, n_per_mop);
}

std::shared_ptr<Shardcontroller> start_shardcontroller(
    const std::string& addr) {
  return start_server<StaticShardController, const std::string&>(addr);
}

std::map<std::string, std::vector<Shard>> make_shard_map(
    std::vector<std::string> servers) {
  std::map<std::string, std::vector<Shard>> res;
  if (!servers.empty()) {
    auto shards = split_into(servers.size());
    for (size_t i = 0; i < shards.size(); i++) {
      res[servers[i]].push_back(shards[i]);
    }
  }
  return res;
}

std::vector<std::string> make_server_addresses(size_t n_servers,
                                               uint64_t start_port) {
  std::vector<std::string> res(n_servers);
  for (size_t i = 0; i < n_servers; i++) {
    auto port_str = std::to_string(start_port + i);
    res[i] = get_host_address(port_str.c_str());
  }
  return res;
}

std::vector<std::vector<std::string>> make_server_chunks(size_t n_servers,
                                                         size_t n_chunks,
                                                         uint64_t start_port) {
  assert(n_servers % n_chunks == 0);

  std::vector<std::vector<std::string>> res(n_chunks);
  size_t servers_per_chunk = n_servers / n_chunks;
  for (size_t i = 0; i < n_chunks; i++) {
    res[i] = make_server_addresses(servers_per_chunk,
                                   start_port + i * servers_per_chunk);
  }
  return res;
}

// Joins/leaves the shardcontroller on that address.
bool test_join(std::shared_ptr<Shardcontroller> sm, const std::string& addr,
               bool success) {
  JoinRequest req{addr};
  JoinResponse res;
  return sm->Join(&req, &res) == success;
}

bool test_leave(std::shared_ptr<Shardcontroller> sm, const std::string& addr,
                bool success) {
  LeaveRequest req{addr};
  LeaveResponse res;
  return sm->Leave(&req, &res) == success;
}

bool test_move(std::shared_ptr<Shardcontroller> sm, const std::string& addr,
               const std::vector<Shard>& shards, bool success) {
  MoveRequest req{addr, shards};
  MoveResponse res;
  return sm->Move(&req, &res) == success;
}

std::map<std::string, std::vector<Shard>> query_config(
    std::shared_ptr<Shardcontroller> sm) {
  QueryRequest req;
  QueryResponse res;
  bool ret = sm->Query(&req, &res);
  ASSERT(ret);

  // sort the shards before returning
  for (auto& [_, shards] : res.config.server_to_shards) {
    sort_shards(shards);
  }

  std::cout << res.config.print();

  return res.config.server_to_shards;
}

bool test_get_impl(std::shared_ptr<ServerConn> conn, const std::string& key,
                   const std::optional<std::string>& value) {
  GetRequest req{key};

  if (!conn->send_request(req)) return false;

  auto res = conn->recv_response();
  // we *should* receive a response...
  if (!res) return false;
  auto* get_res = std::get_if<GetResponse>(&*res);
  // if no value provided, expect an error (i.e. ErrorResponse?)
  if (!value) {
    return get_res == nullptr;
  } else {
    // if we expect a value, check that the value is as expected
    if (get_res) {
      return get_res->value == *value;
    } else {
      // value should be a GetResponse
      return false;
    }
  }
}

bool test_get(const std::string& s_addr, const std::string& key,
              const std::optional<std::string>& value) {
  // Create connection; should not fail if server is open
  std::shared_ptr<ServerConn> conn = connect_to_server(s_addr);
  if (!conn) {
    return false;
  }
  // Loop RETRIES unless failure is desired
  auto timeout = 200ms;
  for (size_t i = 0; i < RETRIES; i++) {
    bool res = test_get_impl(conn, key, value);
    if (res) {
      // if succeeded, return true
      return true;
    } else if (!value) {
      // if failed (i.e. expected failure (so res would've given true), but
      // didn't), return error. Otherwise, continue trying
      return false;
    }
    std::this_thread::sleep_for(timeout);
  }

  return false;
}

bool test_put(const std::string& s_addr, const std::string& key,
              const std::string& value, bool success) {
  // Create connection; should not fail if server is open
  std::shared_ptr<ServerConn> conn = connect_to_server(s_addr);
  if (!conn) {
    return false;
  }

  PutRequest req{key, value};
  // should be able to send request without failure, and receive a response
  if (!conn->send_request(req)) return false;

  auto res = conn->recv_response();
  if (!res) return false;

  auto ret = std::get_if<PutResponse>(&*res) == nullptr;
  return ret == success;
}

bool test_append(const std::string& s_addr, const std::string& key,
                 const std::string& value, bool success) {
  // Create connection; should not fail if server is open
  std::shared_ptr<ServerConn> conn = connect_to_server(s_addr);
  if (!conn) {
    return false;
  }

  AppendRequest req{key, value};
  // should be able to send request without failure, and receive a response
  if (!conn->send_request(req)) return false;

  auto res = conn->recv_response();
  if (!res) return false;

  auto ret = std::get_if<AppendResponse>(&*res) == nullptr;
  return ret == success;
}

bool test_delete(const std::string& s_addr, const std::string& key,
                 const std::optional<std::string>& value) {
  // Create connection; should not fail if server is open
  std::shared_ptr<ServerConn> conn = connect_to_server(s_addr);
  if (!conn) {
    return false;
  }

  DeleteRequest req{key};
  // should be able to send request without failure, and receive a response
  if (!conn->send_request(req)) return false;

  auto res = conn->recv_response();
  if (!res) return false;

  auto delete_res = std::get_if<DeleteResponse>(&*res);
  // if expect no value, delete_res should be nullptr
  if (!delete_res) {
    return value == std::nullopt;
  } else {
    // otherwise, should be the expected value
    return value.has_value() && delete_res->value == *value;
  }
}

bool test_multiget(const std::string& s_addr, const std::string& key,
                   const std::optional<std::vector<std::string>>& value);
bool test_multiput(const std::string& s_addr,
                   const std::vector<std::string>& key,
                   const std::vector<std::string>& value, bool success);

// Helper to calculate throughput for performance tests
double to_throughput(std::chrono::milliseconds time, size_t n_threads,
                     size_t n_keys) {
  return static_cast<double>(n_threads * n_keys) /
         (static_cast<double>(time.count()) / 1000.0);
}
