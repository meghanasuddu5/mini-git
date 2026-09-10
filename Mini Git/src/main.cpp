/// MiniGit — A Git-like Version Control System
///
/// Entry point. Constructs the CLI and runs it.
/// The actual logic lives in the CLI class and its command handlers.

#include "minigit/cli/cli.h"

int main(int argc, char* argv[]) {
    return minigit::CLI(argc, argv).run();
}
