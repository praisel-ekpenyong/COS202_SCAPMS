#pragma once
// Validation helpers: age 0-120; name letters/spaces only;
// loose contact pattern; valid calendar dates; valid clock times.
// Every function throws scapms::utils::ValidationError with a human-readable
// message on the first violated rule, and returns quietly otherwise.
#include <string>

namespace scapms::utils {

// 0..120 inclusive.
void validateAge(int age);

// Non-empty, at most 100 chars, ASCII letters and single spaces only;
// no leading or trailing space.
void validateName(const std::string& name);

// Loose contact pattern: non-empty, at most 32 chars, drawn from digits,
// spaces, '+', '-', '(', ')', and containing at least 5 digits
// (so any plausible phone number passes).
void validateContact(const std::string& contact);

// Exactly "YYYY-MM-DD" and a real calendar date (leap years respected).
void validateDate(const std::string& date);

// Exactly "HH:MM", hours 00-23, minutes 00-59.
void validateTime(const std::string& time);

}  // namespace scapms::utils
