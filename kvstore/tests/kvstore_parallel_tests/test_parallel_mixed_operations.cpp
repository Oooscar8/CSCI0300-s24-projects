
#include <array>
#include <future>
#include <iostream>
#include <string>

#include "test_utils/test_utils.hpp"

static constexpr std::size_t kRandStringLength = 32;
static constexpr std::size_t kNumKeyValPairs = 10'000;
static constexpr std::size_t kNumThreads = 200;
static constexpr std::size_t kNumPerMulti = kNumKeyValPairs / kNumThreads;

int main(int argc, char* argv[]) {
  auto store = make_kvstore(argc, argv);

  auto keys = make_rand_strs(kNumKeyValPairs, kRandStringLength);
  auto vals = make_rand_strs(kNumKeyValPairs, kRandStringLength);

  std::vector<std::future<bool>> threads;

  // Concurrently execute MultiGet/MultiPut requests *and* Get/Put requests
  // If you're failing this test, you should make sure that you're properly
  // synchronizing requests across the multi and single-key operations.
  for (std::size_t i = 0; i < kNumThreads; i++) {
    threads.push_back(std::async(std::launch::async, [&, tid = i]() {
      auto start = tid * kNumPerMulti;
      auto end = start + kNumPerMulti;

      if (tid % 2 == 0) {
        return multiput_range(*store, keys, vals, start, end, kNumPerMulti) &&
               multiget_range(*store, keys, vals, start, end, kNumPerMulti);
      } else {
        return put_range(*store, keys, vals, start, end) &&
               get_range(*store, keys, vals, start, end);
      }
    }));
  }

  bool passed = true;
  for (auto& t : threads) {
    passed &= t.get();
  }
  ASSERT(passed);

  // Once threads are finished, all of the kv pairs should be in the store
  MultiGetRequest multiget_req = MultiGetRequest{.keys = keys};
  MultiGetResponse multiget_res = MultiGetResponse{};
  ASSERT(store->MultiGet(&multiget_req, &multiget_res));
  ASSERT(multiget_res.values == vals);

  return 0;
}