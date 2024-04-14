#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

#include "test_utils/test_utils.hpp"

constexpr std::size_t kRandStringLength = 12;
constexpr std::size_t kNumIters = 10'000;

int main(int argc, char* argv[]) {
  auto store = make_kvstore(argc, argv);

  auto keys = make_rand_strs(kNumIters, kRandStringLength);
  auto vals = make_rand_strs(kNumIters, kRandStringLength);

  auto map = std::unordered_map<std::string, std::string>{};
  for (std::size_t i = 0; i < kNumIters; ++i) {
    auto put_req = PutRequest{.key = keys[i], .value = vals[i]};
    auto put_res = PutResponse{};
    ASSERT(store->Put(&put_req, &put_res));
    map[keys[i]] = vals[i];
  }

  for (std::size_t i = 0; i < kNumIters; ++i) {
    auto get_req = GetRequest{.key = keys[i]};
    auto get_res = GetResponse{};
    ASSERT(store->Get(&get_req, &get_res));
    ASSERT_EQ(get_res.value, vals[i]);
  }

  auto res = store->AllKeys();
  ASSERT_EQ(keys.size(), res.size());

  std::sort(keys.begin(), keys.end());
  std::sort(res.begin(), res.end());
  ASSERT_EQ_VECS(keys, res);

  // Assert that we can make successful requests *after* an AllKeys request
  // succeeds If you fail these assertions, think about what you may be
  // forgetting to do at the end of AllKeys!
  auto put_req = PutRequest{.key = "hello", .value = "world"};
  auto put_res = PutResponse{};
  ASSERT(store->Put(&put_req, &put_res));

  auto get_req_check = GetRequest{.key = "hello"};
  auto get_res_check = GetResponse{};
  ASSERT(store->Get(&get_req_check, &get_res_check));
  ASSERT_EQ(get_res_check.value, "world");
}
