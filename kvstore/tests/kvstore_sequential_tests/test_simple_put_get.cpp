#include "test_utils/test_utils.hpp"

int main(int argc, char* argv[]) {
  auto store = make_kvstore(argc, argv);

  std::string key = "hello";
  std::string val = "world";

  auto put_req = PutRequest{.key = key, .value = val};
  auto put_res = PutResponse{};
  ASSERT(store->Put(&put_req, &put_res));

  auto get_req = GetRequest{.key = key};
  auto get_res = GetResponse{};
  ASSERT(store->Get(&get_req, &get_res));
  ASSERT_EQ(get_res.value, val);

  // Request for nonexistent key should fail
  auto req = GetRequest{"nonexistent"};
  auto res = GetResponse{};
  ASSERT(!store->Get(&req, &res));

  // Should be able to make a successful Get request after a failed one
  get_req = GetRequest{.key = key};
  get_res = GetResponse{};
  ASSERT(store->Get(&get_req, &get_res));
  ASSERT_EQ(get_res.value, val);
}
