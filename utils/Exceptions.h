#pragma once
// Custom exception classes for validation, duplicate IDs, and file operations.
#include <stdexcept>
#include <string>

namespace scapms::utils {

// Thrown when field validation fails.
class ValidationError : public std::runtime_error {
public:
    explicit ValidationError(const std::string& what) : std::runtime_error(what) {}
};

// Thrown when an ID already exists.
class DuplicateIdError : public std::runtime_error {
public:
    explicit DuplicateIdError(const std::string& what) : std::runtime_error(what) {}
};

// Thrown when file I/O operations fail.
class FileError : public std::runtime_error {
public:
    explicit FileError(const std::string& what) : std::runtime_error(what) {}
};

}  // namespace scapms::utils
