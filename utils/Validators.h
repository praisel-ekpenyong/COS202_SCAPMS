#pragma once
// Validation helpers for age, name, contact info, dates, and times.
#include <string>

namespace scapms::utils {

// Checks age is between 0 and 120.
void validateAge(int age);

// Checks name contains only letters and single spaces.
void validateName(const std::string& name);

// Checks contact contains at least 5 digits and valid characters.
void validateContact(const std::string& contact);

// Checks format is YYYY-MM-DD and date is a valid calendar date.
void validateDate(const std::string& date);

// Checks format is HH:MM and time is valid.
void validateTime(const std::string& time);

}  // namespace scapms::utils
