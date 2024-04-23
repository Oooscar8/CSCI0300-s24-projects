#include <array>
#include <future>
#include <iostream>
#include <string>

#include "test_utils/test_utils.hpp"

static constexpr std::size_t kRandStringLength = 32;
static constexpr std::size_t kNumThreads = 20;
static constexpr std::size_t kNumKeyValPairs = 10'000;

int main(int argc, char* argv[]) {
  auto store = make_kvstore(argc, argv);

  // Generate random key-value pairs
  auto keys = make_rand_strs(kNumKeyValPairs, kRandStringLength);
  auto vals = make_rand_strs(kNumKeyValPairs, kRandStringLength);

  auto multiput_req = MultiPutRequest{.keys = keys, .values = vals};
  auto multiput_res = MultiPutResponse{};
  ASSERT(store->MultiPut(&multiput_req, &multiput_res));

  auto n_threads = kNumThreads;
  auto elems_per_thr = kNumKeyValPairs / n_threads;
  auto threads = std::vector<std::future<bool>>{};
  // Make concurrent AllKeys and Delete requests -- if your code is thread-safe,
  // these should succeed
  for (std::size_t i = 0; i < n_threads; ++i) {
    threads.push_back(std::async(std::launch::async, [&, tid = i]() {
      auto start = tid * elems_per_thr;
      auto end = start + elems_per_thr;
      if (tid % 10 == 0) {
        store->AllKeys();
      }
      return del_range(*store, keys, vals, start, end);
    }));
  }

  auto passed = true;
  for (auto& t : threads) {
    passed &= t.get();
  }

  ASSERT(passed);
  return 0;
}
