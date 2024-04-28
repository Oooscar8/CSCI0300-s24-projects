#include "server.hpp"

int KvServer::start() {
  this->is_stopped = false;

  // Initialize KvStore
  // TODO (Part B, Step 2): Change your underlying KvStore to the
  // ConcurrentKvStore!
  this->store = std::make_unique<SimpleKvStore>();

  // Create listener socket, and start client listener
  this->listener_fd = open_listener_socket(address);
  if (this->listener_fd < 0) {
    return -1;
  }
  this->client_listener = std::thread(&KvServer::accept_clients_loop, this);
  cout_color(BLUE, "Listening on: ", this->address);

  // Initialize worker threads
  this->workers.resize(this->n_workers);
  this->conn_queues.resize(this->n_workers);
  this->conn_queue_mtxs.resize(this->n_workers);
  size_t i = 0;
  for (auto&& worker : this->workers) {
    worker = std::thread(&KvServer::work_loop, this, i);
    i++;
  }

  // If shardcontroller address not empty, connect to shardcontroller,
  // tell the shardcontroller that the server has joined, and start query thread
  if (!this->shardcontroller_address.empty()) {
    this->shardcontroller_conn =
        connect_to_server(this->shardcontroller_address);
    if (!this->shardcontroller_conn) {
      close(this->listener_fd);
      return -1;
    }

    this->shardcontroller_querier_conn =
        connect_to_server(this->shardcontroller_address);
    if (!this->shardcontroller_querier_conn) {
      close(this->listener_fd);
      return -1;
    }

    // TODO (Part B, Step 2): Send a join request to the shardcontroller
    // Take a look at the functions provided in this file to see if any of them
    // will help you!

    this->shardcontroller_querier =
        std::thread(&KvServer::process_config_loop, this);
    cout_color(BLUE, "Shardcontroller on: ", this->shardcontroller_address);
  }

  return 0;
}

void KvServer::stop() {
  this->is_stopped = true;

  // Close client listener
  shutdown(this->listener_fd, SHUT_RDWR);
  cout_color(BLUE, "Joining client listener thread...");
  this->client_listener.join();

  // Stop connection queue, and close & join workers
  for (size_t i = 0; i < this->n_workers; i++) {
    auto& queue = this->conn_queues[i];
    this->conn_queue_mtxs[i].lock();
    for (auto&& client : queue) {
      cout_color(BLUE, "Closing connection from ", client->address);
      client->shutdown();
    }
    this->conn_queue_mtxs[i].unlock();
  }
  for (auto&& thr : this->workers) thr.join();

  // If shardcontroller exists, tell shardcontroller the server is leaving,
  // join shardcontroller querier thread, and close shardcontroller connection
  if (!this->shardcontroller_address.empty()) {
    // TODO (Part B, Step 2): Send a leave request to the shardcontroller
    // Take a look at the functions provided in this file to see if any of them
    // will help you!

    cout_color(BLUE, "Joining query shardcontroller thread...");
    this->shardcontroller_querier.join();
    this->shardcontroller_conn->shutdown();
  }
}

bool KvServer::Join() {
  JoinRequest req{this->address};
  if (!this->shardcontroller_conn->send_request(req)) return false;

  std::optional<Response> res = this->shardcontroller_conn->recv_response();
  if (!res) return false;
  if (auto* join_res = std::get_if<JoinResponse>(&*res)) {
    return true;
  }

  return false;
}

bool KvServer::Leave() {
  LeaveRequest req{this->address};
  if (!this->shardcontroller_conn->send_request(req)) return false;

  std::optional<Response> res = this->shardcontroller_conn->recv_response();
  if (!res) return false;
  if (auto* leave_res = std::get_if<LeaveResponse>(&*res)) {
    return true;
  }

  return false;
}

std::optional<QueryResponse> KvServer::query_shardcontroller(
    std::shared_ptr<ServerConn> conn) {
  QueryRequest req{};

  if (!conn->send_request(req)) {
    return std::nullopt;
  }

  auto res = conn->recv_response();
  if (!res) {
    return std::nullopt;
  }
  auto* query_res = std::get_if<QueryResponse>(&*res);
  if (!query_res) {
    return std::nullopt;
  }
  return *query_res;
}

bool KvServer::process_config() {
  // TODO (Part B, Step 2): Implement!

  std::unique_lock lock(this->config_mtx);

  auto res = this->query_shardcontroller(this->shardcontroller_querier_conn);
  if (!res) {
    cerr_color(RED, "Failed to receive query response from shardcontroller.");
    return false;
  }

  // TODO: Update this->config to reflect the result from querying the
  // shardcontroller

  // to_transfer maps server --> [<vector of keys to transfer to server>,
  // <vector of values to transfer to server>]
  // You're free to use a different data structure if you'd
  // like, although note you'll have to change our transferring code if you do
  std::map<std::string, std::array<std::vector<std::string>, 2>> to_transfer;

  // TODO:
  //  1. Get all of the server's key-value pairs (which KvStore functions might
  //  help you with this?)
  //  2. For each pair, check if the server is still responsible for it.
  //  If the server is no longer responsible for the pair,
  //  add the pair to_transfer under the key of the newly-responsible (i.e.,
  //  destination) server and delete the pair from this server's store.

  // NOTE: Comment this in to transfer the keys!
  // // for each server responsible for moved keys:
  // for (auto&& [s, responsible_pairs] : to_transfer) {
  //     while (true) {
  //         // connect to server
  //         std::shared_ptr<ServerConn> conn = connect_to_server(s);
  //         if (!conn) {
  //             cerr_color(RED, "Failed to connect to server ", s);
  //             continue;
  //         }
  //         // make MultiPut request
  //         MultiPutRequest req{responsible_pairs[0], responsible_pairs[1]};
  //         if (!conn->send_request(req)) continue;

  //         // receive response, and check for success
  //         std::optional<Response> res = conn->recv_response();
  //         if (!res) continue;
  //         if (std::get_if<ErrorResponse>(&*res)) continue;
  //         break;
  //     }
  // }

  return true;
}

/* ==================================================*/
/* === INTERNALS: DO NOT MODIFY BELOW THIS LINE ===  */
/* ==================================================*/

void KvServer::accept_clients_loop() {
  size_t next_worker = 0;
  // While the server is not stopped, accept clients from the listener socket,
  // then add them to the work queue.
  while (!this->is_stopped.load()) {
    std::shared_ptr<ClientConn> client = accept_client(this->listener_fd);
    if (!client) {
      return;
    }
    cout_color(BLUE, "Received client connection from ", client->address,
               " on socket ", client->fd);
    this->conn_queue_mtxs[next_worker].lock();
    this->conn_queues[next_worker].push_back(client);
    this->conn_queue_mtxs[next_worker].unlock();
    next_worker = (next_worker + 1) % this->n_workers;
  }
}

void KvServer::work_loop(size_t worker_id) {
  // Each worker thread will run this function. While the server is not stopped,
  // pop an accepted connection off of the work queue, and process client
  // requests until the client closes the connection.
  while (!this->is_stopped) {
    std::shared_ptr<ClientConn> client;
    // if this returns false, queue stopped
    this->conn_queue_mtxs[worker_id].lock();
    if (!this->conn_queues[worker_id].empty()) {
      client = this->conn_queues[worker_id].front();
      this->conn_queues[worker_id].pop_front();
      this->conn_queue_mtxs[worker_id].unlock();
    } else {
      this->conn_queue_mtxs[worker_id].unlock();
      continue;
    }

    while (true) {
      std::optional<Request> req = client->recv_request();
      if (!req) {
        client->close();
        break;
      }
      Response res = this->process_request(*req);
      if (auto* error_res = std::get_if<ErrorResponse>(&res)) {
        cerr_color(RED, "Request on server ", this->address,
                   " failed: ", error_res->msg);
      }
      if (!client->send_response(res)) {
        client->close();
        break;
      }
    }
  }
}

bool KvServer::responsible_for(const std::string& key) {
  // For Concurrent Store, no shardcontroller exists, so no-op
  if (this->shardcontroller_address.empty()) return true;

  std::shared_lock lock(this->config_mtx);
  auto server = this->config.get_server(key);
  if (!server) return false;
  return *server == this->address;
}

bool KvServer::responsible_for(const std::vector<std::string>& keys) {
  // For Concurrent Store, no shardcontroller exists, so no-op
  if (this->shardcontroller_address.empty()) return true;

  std::shared_lock lock(this->config_mtx);
  for (auto&& k : keys) {
    auto server = this->config.get_server(k);
    if (!server) return false;
    if (*server != this->address) return false;
  }
  return true;
}

Response KvServer::process_request(Request req) {
  Response res;
  if (auto* get_req = std::get_if<GetRequest>(&req)) {
    bool responsible = this->responsible_for(get_req->key);
    GetResponse get_res;
    if (responsible && this->store->Get(get_req, &get_res)) {
      res = get_res;
    } else {
      res = ErrorResponse{
          !responsible ? std::string("server not responsible for key")
                       : std::string("key does not exist in the KVStore")};
    }
  } else if (auto* put_req = std::get_if<PutRequest>(&req)) {
    bool responsible = this->responsible_for(put_req->key);
    PutResponse put_res;
    if (responsible && this->store->Put(put_req, &put_res)) {
      res = put_res;
    } else {
      // Put should never fail
      res = ErrorResponse{!responsible
                              ? std::string("server not responsible for key")
                              : std::string("internal KVStore error")};
    }
  } else if (auto* append_req = std::get_if<AppendRequest>(&req)) {
    bool responsible = this->responsible_for(append_req->key);
    AppendResponse append_res;
    if (responsible && this->store->Append(append_req, &append_res)) {
      res = append_res;
    } else {
      res = ErrorResponse{!responsible
                              ? std::string("server not responsible for key")
                              : std::string("internal KVStore error")};
    }
  } else if (auto* delete_req = std::get_if<DeleteRequest>(&req)) {
    bool responsible = this->responsible_for(delete_req->key);
    DeleteResponse delete_res;
    if (responsible && this->store->Delete(delete_req, &delete_res)) {
      res = delete_res;
    } else {
      res = ErrorResponse{
          !responsible ? std::string("server not responsible for key")
                       : std::string("key does not exist in the KVStore")};
    }
  } else if (auto* multiget_req = std::get_if<MultiGetRequest>(&req)) {
    bool responsible = this->responsible_for(multiget_req->keys);
    MultiGetResponse multiget_res;
    if (responsible && this->store->MultiGet(multiget_req, &multiget_res)) {
      res = multiget_res;
    } else {
      res = ErrorResponse{
          !responsible ? std::string("server not responsible for key(s)")
                       : std::string("key(s) do not exist in the KVStore")};
    }
  } else if (auto* multiput_req = std::get_if<MultiPutRequest>(&req)) {
    bool responsible = this->responsible_for(multiput_req->keys);
    MultiPutResponse multiput_res;
    if (responsible && this->store->MultiPut(multiput_req, &multiput_res)) {
      res = multiput_res;
    } else {
      res = ErrorResponse{!responsible
                              ? std::string("server not responsible for key(s)")
                              : std::string("internal KVStore error")};
    }
  } else {
    throw std::logic_error{"invalid variant!"};
  }
  return res;
}

void KvServer::process_config_loop() {
  int failure_count = 0;
  while (!this->is_stopped) {
    if (this->process_config()) {
      failure_count = 0;
    } else {
      failure_count += 1;
      // process_config can fail if the destination server for a move hasn't
      // queried the shardcontroller yet so we tolerate some failures before
      // breaking
      if (failure_count > 100) {
        break;
      }
    }
    std::this_thread::sleep_for(250ms);
  }
}

ShardControllerConfig KvServer::get_config() {
  return this->config;
}

std::map<std::string, std::string> KvServer::all_kvpairs() {
  auto keys = this->store->AllKeys();
  std::map<std::string, std::string> map;
  for (auto&& k : keys) {
    auto req = GetRequest{k};
    auto res = GetResponse{};
    // Only add if key still exists
    if (this->store->Get(&req, &res)) {
      map[k] = res.value;
    }
  }

  return map;
}
