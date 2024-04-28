#include <algorithm>
#include <cassert>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "common/shard.hpp"
#include "net/network_helpers.hpp"
#include "server/server.hpp"
#include "test_utils/test_utils.hpp"

// for simplicity
using namespace std;

constexpr size_t N_TRIES = 5;
constexpr size_t N_THREADS = 1'000;

int main() {
  string sm_addr = get_host_address("8080");

  shared_ptr<Shardcontroller> sm = start_shardcontroller(sm_addr);

  string server = "server:123";

  int success_count = 0;
  mutex mtx;
  std::vector<std::thread> thrs(N_THREADS);

  for (size_t j = 0; j < N_TRIES; j++) {
    // Launch N_THREADS to join the same server; should succeed only once
    // (whichever thread gets there first should succeed, rest should fail b/c
    // config already contains server)
    for (size_t i = 0; i < N_THREADS; i++) {
      thrs[i] = thread(
          [&](string server, int* success_count) {
            JoinRequest req{server};
            JoinResponse res;
            if (sm->Join(&req, &res)) {
              mtx.lock();
              *success_count += 1;
              mtx.unlock();
            }
          },
          server, &success_count);
    }

    for (size_t i = 0; i < N_THREADS; i++) {
      thrs[i].join();
    }

    ASSERT_EQ(success_count, 1);

    success_count = 0;

    // Launch N_THREADS to leave the same server; should succeed only once
    // (whichever thread gets there first should succeed, rest should fail b/c
    // config no longer has server)
    for (size_t i = 0; i < N_THREADS; i++) {
      thrs[i] = thread(
          [&](string server, int* success_count) {
            LeaveRequest req{server};
            LeaveResponse res;
            if (sm->Leave(&req, &res)) {
              mtx.lock();
              *success_count += 1;
              mtx.unlock();
            }
          },
          server, &success_count);
    }

    for (size_t i = 0; i < N_THREADS; i++) {
      thrs[i].join();
    }

    ASSERT_EQ(success_count, 1);
    success_count = 0;
  }

  cout_color(GREEN, "Test passed!");

  sm->stop();

  return 0;
}
