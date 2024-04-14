#include "server/server.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include "kvstore/kvstore.hpp"
#include "repl/repl.hpp"
#include "server/cmd/joincommand.hpp"
#include "server/cmd/leavecommand.hpp"
#include "server/cmd/printcommand.hpp"

int main(int argc, char* argv[]) {
  if (argc < 2 || argc > 4) {
    cerr_color(RED,
               "\nIf on Concurrent Store:\n"
               "\t./server <port> [n_workers]\n"
               "If on Distributed Store:\n"
               "\t./server <port> <shardcontroller hostname:port> [n_workers]");
    return EXIT_FAILURE;
  }

  std::shared_ptr<KvServer> server;

  std::string addr = get_host_address(argv[1]);
  std::string shardcontroller_addr;
  uint64_t n_workers = N_WORKERS;

  if (argc == 3) {
    // if second argument contains a colon, then it's a shardcontroller address
    if (std::string(argv[2]).find(":") != std::string::npos) {
      shardcontroller_addr = std::string(argv[2]);
    } else {
      // otherwise, we're running Concurrent Store and specified the number of
      // workers
      n_workers = std::stoi(argv[2]);
    }
  } else if (argc == 4) {
    // Running Distributed Store with both shardcontroller and n_workers
    // specified
    shardcontroller_addr = std::string(argv[2]);
    n_workers = std::stoi(argv[3]);
  }

  // If no shardcontroller address specified, Concurrent Store; otherwise,
  // Distributed Store
  if (shardcontroller_addr.empty()) {
    server = std::make_shared<KvServer>(addr, n_workers);
  } else {
    server = std::make_shared<KvServer>(addr, shardcontroller_addr, n_workers);
  }

  int ret = server->start();
  if (ret < 0) {
    exit(EXIT_FAILURE);
  }

  Repl repl;
  if (argc == 4) {
    // if Distributed Store, add shardcontroller commands
    JoinCommand jc{server};
    repl.add_command(jc);
    LeaveCommand lc{server};
    repl.add_command(lc);
  }
  // - `print <store|config>` (display store/config)
  PrintCommand pc{server};
  repl.add_command(pc);

  repl.run();

  server->stop();
}
