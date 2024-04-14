#ifndef KVSERVER_HPP
#define KVSERVER_HPP

#include <array>
#include <chrono>
#include <deque>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <thread>
#include <utility>

#include "kvstore/concurrent_kvstore.hpp"
#include "kvstore/kvstore.hpp"
#include "kvstore/simple_kvstore.hpp"
#include "net/network_conn.hpp"
#include "net/network_helpers.hpp"
#include "net/network_messages.hpp"

#define N_WORKERS 5

using namespace std::chrono;

class KvServer {
 public:
  explicit KvServer(const std::string& address, uint64_t n_workers)
      : address(address), shardcontroller_address(), n_workers(n_workers) {
  }
  explicit KvServer(const std::string& address,
                    const std::string& shardcontroller_addr, uint64_t n_workers)
      : address(address),
        shardcontroller_address(shardcontroller_addr),
        n_workers(n_workers) {
  }
  ~KvServer() {
    if (!this->is_stopped) {
      this->stop();
    }
  }

  // Starts the KvServer.
  int start();
  void stop();

  // Shardcontroller functions
  bool Join();

  bool Leave();

  // For debugging purposes, get all key-value pairs from the store. Note that
  // this operation is not atomic; do not attempt to use this to atomically
  // retrieve key-value pairs!
  std::map<std::string, std::string> all_kvpairs();

  // For debugging purposes, get the shardcontroller config from the server.
  ShardControllerConfig get_config();

  // For testing purposes, make ServerTest a friend of KvServer
  // so that ServerTest can access KvServer's private fields
  friend class ServerTest;

  KvServer(const KvServer&) = delete;
  KvServer& operator=(const KvServer&) = delete;

 private:
  /* ========================================================================*/
  /* === NOTE: You do not (and should not) need to modify or add any fields. */
  /* ========================================================================*/

  // NOTE: You'll need the fields below for your process_config implementation!

  // The address on which the server is listening.
  std::string address;

  // Internal key-value store.
  std::unique_ptr<KvStore> store;

  // Persistent shardcontroller connection.
  std::shared_ptr<ServerConn> shardcontroller_conn;

  // Shardcontroller configuration.
  ShardControllerConfig config;

  // mutex to synchronize access to the config
  std::shared_mutex config_mtx;

  // END of fields you need for process_config

  // An atomic, thread-safe boolean to denote whether the server has been
  // stopped.
  std::atomic<bool> is_stopped;

  // Listener socket for incoming client connections.
  int listener_fd;
  // Thread that listens for client connections and accepts them.
  std::thread client_listener;

  // Thread that periodically queries the shardcontroller for the current
  // configuration.
  std::thread shardcontroller_querier;  // bro this name goofy
  std::shared_ptr<ServerConn> shardcontroller_querier_conn;

  // Vector of worker threads.
  std::vector<std::thread> workers;

  // Per-worker queues of client connections to handle.
  std::vector<std::deque<std::shared_ptr<ClientConn>>> conn_queues;
  std::deque<std::mutex> conn_queue_mtxs;

  // The address on which the shardcontroller is listening.
  std::string shardcontroller_address;

  // Number of worker threads.
  uint64_t n_workers;

  /**
   * In a loop, accept client connections, then pass each connection into the
   * work queue of client connections to process.
   *
   * Exits when the server has been stopped.
   */
  void accept_clients_loop();

  /**
   * In a loop, pop a client connection from the work queue and process a
   * request from it. The argument specifies the worker thread ID running the
   * loop. Exits when the server has been stopped.
   */
  void work_loop(size_t worker_id);

  /**
   * Check whether this server is responsible for a key (or list of keys).
   */
  bool responsible_for(const std::string& key);
  bool responsible_for(const std::vector<std::string>& keys);

  /**
   * Query the shardcontroller, then update the config and move outdated pairs
   * to updated servers.
   */
  bool process_config();

  /**
   * Process an incoming request: parse its request type, call its appropriate
   * handler (Get, Put, etc.), then get a response.
   */
  Response process_request(Request req);

  // Extracts a query response from the shardcontroller, or an std::nullopt if
  // one doesn't exist. You might need this when implementing process_config!
  std::optional<QueryResponse> query_shardcontroller(
      std::shared_ptr<ServerConn> conn);

  // Wrapper function that calls process_config periodically.
  void process_config_loop();
};

#endif /* end of include guard */
