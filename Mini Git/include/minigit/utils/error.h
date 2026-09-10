#pragma once

#include <stdexcept>
#include <string>

namespace minigit {

/// Base exception for all MiniGit errors.
/// Inherits from std::runtime_error so callers can catch broadly
/// or narrowly as needed.
class MiniGitException : public std::runtime_error {
public:
    explicit MiniGitException(const std::string& message)
        : std::runtime_error(message) {}
};

/// Thrown when a repository operation fails.
/// Examples: init in a non-writable directory, repo not found,
/// corrupted .minigit structure.
class RepositoryException : public MiniGitException {
public:
    explicit RepositoryException(const std::string& message)
        : MiniGitException("repository error: " + message) {}
};

/// Thrown when a CLI command is malformed or unsupported.
/// Examples: unknown command, missing required argument.
class CommandException : public MiniGitException {
public:
    explicit CommandException(const std::string& message)
        : MiniGitException("command error: " + message) {}
};

/// Thrown when a filesystem operation fails unexpectedly.
/// Examples: cannot create directory, cannot read file.
class FilesystemException : public MiniGitException {
public:
    explicit FilesystemException(const std::string& message)
        : MiniGitException("filesystem error: " + message) {}
};

} // namespace minigit
