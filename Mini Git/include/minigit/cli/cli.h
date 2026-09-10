#pragma once

#include <string>
#include <vector>

namespace minigit {

/// The CLI class parses command-line arguments and dispatches
/// to the appropriate handler function.
///
/// Usage pattern:
///   int main(int argc, char* argv[]) {
///       return CLI(argc, argv).run();
///   }
///
/// Commands are dispatched by name. For Phase 1, only "init" is
/// implemented. All others print a placeholder message.
class CLI {
public:
    /// Construct from argc/argv (as received by main()).
    CLI(int argc, char* argv[]);

    /// Parse the command and execute it.
    /// Returns 0 on success, 1 on error.
    int run();

private:
    // ── Command handlers ────────────────────────────────────────────
    int handleInit();
    int handleAdd();
    int handleStatus();
    int handleCommit();
    int handleLog();
    int handleShow();
    int handleBranch();
    int handleCheckout();
    int handleDiff();
    int handleMerge();

    // ── Helpers ─────────────────────────────────────────────────────
    void printUsage() const;
    void printVersion() const;

    /// The program name (argv[0]).
    std::string programName_;

    /// The command name (e.g., "init", "add", "commit").
    std::string command_;

    /// Remaining arguments after the command name.
    std::vector<std::string> args_;
};

} // namespace minigit
