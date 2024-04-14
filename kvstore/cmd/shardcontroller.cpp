#include "shardcontroller/shardcontroller.hpp"

#include <iostream>

#include "repl/repl.hpp"
#include "shardcontroller/cmd/querycommand.hpp"
#include "shardcontroller/static_shardcontroller.hpp"

int main(int argc, char* argv[]) {
  if (argc != 2) {
    cerr_color(RED, "Usage: ./shardcontroller <PORT>");
    exit(EXIT_FAILURE);
  }

  // Get shardcontroller address, for servers to connect
  std::string addr = get_host_address(argv[1]);
  std::shared_ptr<Shardcontroller> shardcontroller =
      std::make_shared<StaticShardController>(addr);

  int ret = shardcontroller->start();
  if (ret < 0) {
    cerr_color(RED, "Failed to start Shardcontroller on address ", addr);
    exit(EXIT_FAILURE);
  }

  Repl repl;
  // Add query, ... commands
  QueryCommand qc{shardcontroller};
  repl.add_command(qc);

  repl.run();

  shardcontroller->stop();

  return 0;
}
