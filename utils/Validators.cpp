#include "utils/Validators.h"

#include "utils/Exceptions.h"

#include <cctype>
#include <string>

namespace scapms::utils {
namespace {

bool isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int daysInMonth(int year, int month) {
    switch (month) {
        case 1: case 3: case 5: case 7: case 8: case 10: case 12:
            return 31;
        case 4: case 6: case 9: case 11:
            return 30;
        case 2:
            return isLeapYear(year) ? 29 : 28;
        default:
            return 0;
    }
}

// Parses two digits from string at given position
int twoDigits(const std::string& s, std::size_t pos) {
    if (pos + 1 >= s.size() ||
        !std::isdigit(static_cast<unsigned char>(s[pos])) ||
        !std::isdigit(static_cast<unsigned char>(s[pos + 1]))) {
        return -1;
    }
    return (s[pos] - '0') * 10 + (s[pos + 1] - '0');
}

}  // namespace

void validateAge(int age) {
    if (age < 0 || age > 120) {
        throw ValidationError("age must be between 0 and 120, got " +
                              std::to_string(age));
    }
}

void validateName(const std::string& name) {
    if (name.empty()) {
        throw ValidationError("name must not be empty");
    }
    if (name.size() > 100) {
        throw ValidationError("name must be at most 100 characters");
    }
    if (name.front() == ' ' || name.back() == ' ') {
        throw ValidationError("name must not start or end with a space: \"" +
                              name + "\"");
    }
    bool previousWasSpace = false;
    for (char c : name) {
        if (c == ' ') {
            if (previousWasSpace) {
                throw ValidationError("name must not contain double spaces: \"" +
                                      name + "\"");
            }
            previousWasSpace = true;
            continue;
        }
        if (!std::isalpha(static_cast<unsigned char>(c))) {
            throw ValidationError(
                "name may contain letters and spaces only, got \"" + name + "\"");
        }
        previousWasSpace = false;
    }
}

void validateContact(const std::string& contact) {
    if (contact.empty()) {
        throw ValidationError("contact must not be empty");
    }
    if (contact.size() > 32) {
        throw ValidationError("contact must be at most 32 characters");
    }
    int digits = 0;
    for (char c : contact) {
        if (std::isdigit(static_cast<unsigned char>(c))) {
            ++digits;
            continue;
        }
        const bool allowed =
            c == ' ' || c == '+' || c == '-' || c == '(' || c == ')';
        if (!allowed) {
            throw ValidationError(
                "contact may contain digits, spaces, '+', '-', '(', ')' only: \"" +
                contact + "\"");
        }
    }
    if (digits < 5) {
        throw ValidationError("contact must contain at least 5 digits: \"" +
                              contact + "\"");
    }
}

void validateDate(const std::string& date) {
    // Check format is YYYY-MM-DD
    if (date.size() != 10 || date[4] != '-' || date[7] != '-') {
        throw ValidationError("date must have format YYYY-MM-DD, got \"" +
                              date + "\"");
    }
    for (std::size_t i = 0; i < date.size(); ++i) {
        if (i == 4 || i == 7) {
            continue;
        }
        if (!std::isdigit(static_cast<unsigned char>(date[i]))) {
            throw ValidationError("date must have format YYYY-MM-DD, got \"" +
                                  date + "\"");
        }
    }
    const int year = (date[0] - '0') * 1000 + (date[1] - '0') * 100 +
                     (date[2] - '0') * 10 + (date[3] - '0');
    const int month = twoDigits(date, 5);
    const int day = twoDigits(date, 8);
    if (month < 1 || month > 12) {
        throw ValidationError("date has invalid month: \"" + date + "\"");
    }
    if (day < 1 || day > daysInMonth(year, month)) {
        throw ValidationError("date has invalid day for its month: \"" +
                              date + "\"");
    }
}

void validateTime(const std::string& time) {
    if (time.size() != 5 || time[2] != ':') {
        throw ValidationError("time must have format HH:MM, got \"" + time + "\"");
    }
    const int hour = twoDigits(time, 0);
    const int minute = twoDigits(time, 3);
    if (hour < 0 || minute < 0) {
        throw ValidationError("time must have format HH:MM, got \"" + time + "\"");
    }
    if (hour > 23) {
        throw ValidationError("time hour must be 00-23, got \"" + time + "\"");
    }
    if (minute > 59) {
        throw ValidationError("time minute must be 00-59, got \"" + time + "\"");
    }
}

}  // namespace scapms::utils
