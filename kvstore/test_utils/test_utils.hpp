#ifndef TEST_UTILS_HPP
#define TEST_UTILS_HPP

#include <algorithm>
#include <chrono>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

#include "common/shard.hpp"
#include "kvstore/concurrent_kvstore.hpp"
#include "kvstore/simple_kvstore.hpp"
#include "net/network_helpers.hpp"
#include "net/server_commands.hpp"
#include "net/shardcontroller_commands.hpp"
#include "server/server.hpp"
#include "shardcontroller/shardcontroller.hpp"
#include "shardcontroller/static_shardcontroller.hpp"

constexpr int RETRIES = 10;

/**
 * @tparam Type of Server we're creating
 * @tparam Args types of arguments to the Server's constructor
 * @param args args to Server constructor
 *
 * This function creates a server.
 */
template <typename Server, typename... Args>
std::shared_ptr<Server> start_server(Args&&... args) {
  std::shared_ptr<Server> server =
      std::make_shared<Server>(std::forward<Args>(args)...);
  server->start();
  return std::move(server);
}

template <typename Server, typename... Args>
void start_server_in_thread(Args&&... args) {
  std::thread thr(start_server<Server, Args...>, std::forward<Args>(args)...);
  thr.detach();
  // sleep to allow service to start
  std::this_thread::sleep_for(100ms);
}

template <typename Functor, typename Iterable>
void execute_in_parallel(Functor func, std::vector<Iterable> chunks) {
  std::vector<std::thread> thrs(chunks.size());
  for (size_t i = 0; i < chunks.size(); i++) {
    thrs[i] = std::thread(func, chunks[i]);
  }
  for (auto&& thr : thrs) {
    thr.join();
  }
}

// ===== Testing asserts

#define ASSERT(test_expr)                                                  \
  do {                                                                     \
    if (!(test_expr)) {                                                    \
      std::stringstream stream;                                            \
      stream << RED << "Assert failed in file " << __FILE__ << " on line " \
             << __LINE__ << NC << "\n";                                    \
      std::cout << stream.str();                                           \
      exit(1);                                                             \
    }                                                                      \
  } while (0);

#define ASSERT_EQ(expr1, expr2)                                            \
  do {                                                                     \
    auto lhs = (expr1);                                                    \
    auto rhs = (expr2);                                                    \
    if (lhs != rhs) {                                                      \
      std::stringstream stream;                                            \
      stream << RED << "Assert failed in file " << __FILE__ << " on line " \
             << __LINE__ << ": expected first argument " << lhs            \
             << " to equal second argument " << rhs << NC << "\n";         \
      std::cout << stream.str();                                           \
      exit(1);                                                             \
    }                                                                      \
  } while (0);

#define ASSERT_EQ_VECS(expr1, expr2)                                       \
  do {                                                                     \
    auto lhs = (expr1);                                                    \
    auto rhs = (expr2);                                                    \
    if (lhs != rhs) {                                                      \
      std::stringstream stream;                                            \
      stream << RED << "Assert failed in file " << __FILE__ << " on line " \
             << __LINE__ << ": \n expected first vector: ";                \
      for (auto item : lhs) {                                              \
        stream << RED << item << " ";                                      \
      }                                                                    \
      stream << RED << "\n to equal second vector: ";                      \
      for (auto item : rhs) {                                              \
        stream << RED << item << " ";                                      \
      }                                                                    \
      stream << NC << "\n";                                                \
      std::cout << stream.str();                                           \
      exit(1);                                                             \
    }                                                                      \
  } while (0);

#define ASSERT_EQ_CONFIGS(expr1, expr2)                                    \
  do {                                                                     \
    auto lhs = (expr1);                                                    \
    auto rhs = (expr2);                                                    \
    if (lhs != rhs) {                                                      \
      std::stringstream stream;                                            \
      stream << RED << "Assert failed in file " << __FILE__ << " on line " \
             << __LINE__ << ": \n\nexpected first config:";                \
      for (auto&& [server, shards] : lhs) {                                \
        stream << RED << "\n" << server << ": ";                           \
        for (auto&& shard : shards) {                                      \
          stream << shard;                                                 \
          if (shard != shards.back()) stream << ", ";                      \
        }                                                                  \
      }                                                                    \
      stream << RED << "\n\nto equal second config:";                      \
      for (auto&& [server, shards] : rhs) {                                \
        stream << RED << "\n" << server << ": ";                           \
        for (auto&& shard : shards) {                                      \
          stream << shard;                                                 \
          if (shard != shards.back()) stream << ", ";                      \
        }                                                                  \
      }                                                                    \
      stream << NC << "\n";                                                \
      std::cout << stream.str();                                           \
      exit(1);                                                             \
    }                                                                      \
  } while (0);

#define TEST(test_fn, ...)                       \
  do {                                           \
    std::cout << "testing " << #test_fn << "\n"; \
    test_fn(__VA_ARGS__);                        \
  } while (0);

// ===== Miscellaneous utilities

inline std::string random_string(
    std::size_t str_len,
    std::string valid_chars =
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz") {
  std::random_device rd;
  std::mt19937 generator(rd());

  std::shuffle(valid_chars.begin(), valid_chars.end(), generator);

  assert(str_len <= valid_chars.size());
  return valid_chars.substr(0, str_len);
}

inline std::vector<std::string> make_rand_strs(
    std::size_t num_strs, std::size_t str_len,
    std::string valid_chars =
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz") {
  std::unordered_set<std::string> strset;
  strset.reserve(num_strs);
  while (strset.size() < num_strs) {
    strset.insert(random_string(str_len, valid_chars));
  }

  std::vector<std::string> strs(strset.begin(), strset.end());
  return strs;
}
/**
 * @brief Generate random strings with an index appended.
 *
 * @param num_strs number of strings to generate
 * @param str_len length of each string
 * @param index index to append to each string
 * @return std::vector<std::string> a vector of random strings
 */
inline std::vector<std::string> make_pseudo_rand_str(std::size_t num_strs,
                                                     std::size_t str_len,
                                                     std::size_t index) {
  std::vector<std::string> strs;
  strs.reserve(num_strs);
  for (size_t i = 0; i < num_strs; i++) {
    strs.push_back(random_string(str_len) + std::to_string(index));
  }
  return strs;
}

// Helper to calculate throughput for performance tests
double to_throughput(std::chrono::milliseconds time, size_t n_threads,
                     size_t n_keys);

// ===== KvStore testing utilities

inline std::unique_ptr<KvStore> make_kvstore(int argc, char* argv[]) {
  if (argc == 2) {
    auto type = std::string(argv[1]);
    if (type == "simple") {
      return std::make_unique<SimpleKvStore>();
    } else if (type == "concurrent") {
      return std::make_unique<ConcurrentKvStore>();
    } else {
      cerr_color(RED, "Argument must be \"simple\" or \"concurrent\"");
      exit(EXIT_FAILURE);
    }
  } else {
    // Default to ConcurrentKvStore
    return std::make_unique<ConcurrentKvStore>();
  }
}

bool put_range(KvStore& store, const std::vector<std::string>& keys,
               const std::vector<std::string>& vals, std::size_t start,
               std::size_t end);

bool get_range(KvStore& store, const std::vector<std::string>& keys,
               const std::vector<std::string>& vals, std::size_t start,
               std::size_t end);

bool del_range(KvStore& store, const std::vector<std::string>& keys,
               const std::vector<std::string>& vals, std::size_t start,
               std::size_t end);

bool push_get_delete_range(KvStore& store, const std::vector<std::string>& keys,
                           const std::vector<std::string>& vals,
                           std::size_t start, std::size_t end);

bool multiput_range(KvStore& store, const std::vector<std::string>& keys,
                    const std::vector<std::string>& vals, std::size_t start,
                    std::size_t end, std::size_t n_per_mput);

bool multiget_range(KvStore& store, const std::vector<std::string>& keys,
                    const std::vector<std::string>& vals, std::size_t start,
                    std::size_t end, std::size_t n_per_mget);

bool multiput_multiget_range(KvStore& store,
                             const std::vector<std::string>& keys,
                             const std::vector<std::string>& vals,
                             std::size_t start, std::size_t end,
                             std::size_t n_per_mop);

// ===== Shardcontroller testing utilities

std::shared_ptr<Shardcontroller> start_shardcontroller(const std::string& addr);

// Makes a map of server->shards from a vector of servers.
std::map<std::string, std::vector<Shard>> make_shard_map(
    std::vector<std::string> servers);

// Makes n_server addresses of the form <hostname>:<start_port + i>.
std::vector<std::string> make_server_addresses(size_t n_servers,
                                               uint64_t start_port = 10000);
// Same as above, except chunks the servers into n_chunks. n_servers must be
// divisible by n_chunks.
std::vector<std::vector<std::string>> make_server_chunks(
    size_t n_servers, size_t n_chunks, uint64_t start_port = 10000);

// Joins/leaves the shardcontroller on that address.
bool test_join(std::shared_ptr<Shardcontroller> sm, const std::string& addr,
               bool success = true);
bool test_leave(std::shared_ptr<Shardcontroller> sm, const std::string& addr,
                bool success = true);

// Moves the shards to the address.
bool test_move(std::shared_ptr<Shardcontroller> sm, const std::string& addr,
               const std::vector<Shard>& shards, bool success = true);

// Queries the shardcontroller; returns the server_to_shards map in the config
std::map<std::string, std::vector<Shard>> query_config(
    std::shared_ptr<Shardcontroller> sm);

// ===== ShardKv testing utilities

// Gets/puts/appends/deletes/multigets/multiputs a key (+ value) on the
// specified KvServer.
bool test_get(const std::string& s_addr, const std::string& key,
              const std::optional<std::string>& value);
bool test_put(const std::string& s_addr, const std::string& key,
              const std::string& value, bool success = true);
bool test_append(const std::string& s_addr, const std::string& key,
                 const std::string& value, bool success = true);
bool test_delete(const std::string& s_addr, const std::string& key,
                 const std::optional<std::string>& value);
bool test_multiget(const std::string& s_addr, const std::string& key,
                   const std::optional<std::vector<std::string>>& value);
bool test_multiput(const std::string& s_addr,
                   const std::vector<std::string>& key,
                   const std::vector<std::string>& value, bool success = true);

#endif /* end of include guard */
