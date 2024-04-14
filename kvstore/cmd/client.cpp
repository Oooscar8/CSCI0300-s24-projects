// Clients
#include "client/client.hpp"

#include "client/shardkv_client.hpp"
#include "client/simple_client.hpp"

// Commands
#include "client/cmd/appendcommand.hpp"
#include "client/cmd/deletecommand.hpp"
#include "client/cmd/gdpr_deletecommand.hpp"
#include "client/cmd/getcommand.hpp"
#include "client/cmd/movecommand.hpp"
#include "client/cmd/multigetcommand.hpp"
#include "client/cmd/multiputcommand.hpp"
#include "client/cmd/putcommand.hpp"
#include "client/cmd/querycommand.hpp"
#include "common/color.hpp"
#include "repl/repl.hpp"

int main(int argc, char* argv[]) {
  if (argc != 2) {
#ifdef SIMPLE_CLIENT
    cerr_color(RED, "Usage: ./simple_client <server hostname:port>");
#else
    cerr_color(RED, "Usage: ./client <shardcontroller hostname:port>");
#endif
    exit(EXIT_FAILURE);
  }

#ifdef SIMPLE_CLIENT
  std::shared_ptr<SimpleClient> client =
      std::make_shared<SimpleClient>(argv[1]);
#else
  std::shared_ptr<ShardKvClient> client =
      std::make_shared<ShardKvClient>(argv[1]);
#endif

  Repl repl;

#ifndef SIMPLE_CLIENT
  // Shardcontroller command
  ClientQueryCommand qc{client};
  repl.add_command(qc);
  MoveCommand mc{client};
  repl.add_command(mc);
#endif

  // KvServer commands
  GetCommand gc{client};
  repl.add_command(gc);
  PutCommand pc{client};
  repl.add_command(pc);
  AppendCommand ac{client};
  repl.add_command(ac);
  DeleteCommand dc{client};
  repl.add_command(dc);
  MultiGetCommand mgc{client};
  repl.add_command(mgc);
  MultiPutCommand mpc{client};
  repl.add_command(mpc);
  GDPRDeleteCommand gdel{client};
  repl.add_command(gdel);

  repl.run();

  return 0;
}
