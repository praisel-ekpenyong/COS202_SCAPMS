#include "utils/IDGenerator.h"

namespace scapms::utils {
namespace {

// Renders "P-0007": the prefix, a hyphen, then the number zero-padded to 4
// digits (wider naturally once the counter passes 9999).
std::string formatId(char prefix, unsigned number) {
    std::string id;
    id += prefix;
    id += '-';
    const std::string digits = std::to_string(number);
    for (std::size_t i = digits.size(); i < 4; ++i) {
        id += '0';
    }
    id += digits;
    return id;
}

}  // namespace

std::string IDGenerator::nextPatientId() {
    return formatId('P', ++patientCounter_);
}

std::string IDGenerator::nextAppointmentId() {
    return formatId('A', ++appointmentCounter_);
}

unsigned IDGenerator::patientCounter() const {
    return patientCounter_;
}

unsigned IDGenerator::appointmentCounter() const {
    return appointmentCounter_;
}

void IDGenerator::restorePatientCounter(unsigned highestIssued) {
    patientCounter_ = highestIssued;
}

void IDGenerator::restoreAppointmentCounter(unsigned highestIssued) {
    appointmentCounter_ = highestIssued;
}

bool isPatientId(const std::string& s) {
    if (s.size() < 6 || s[0] != 'P' || s[1] != '-') {
        return false;
    }
    for (std::size_t i = 2; i < s.size(); ++i) {
        if (s[i] < '0' || s[i] > '9') {
            return false;
        }
    }
    return true;
}

bool isAppointmentId(const std::string& s) {
    if (s.size() < 6 || s[0] != 'A' || s[1] != '-') {
        return false;
    }
    for (std::size_t i = 2; i < s.size(); ++i) {
        if (s[i] < '0' || s[i] > '9') {
            return false;
        }
    }
    return true;
}

}  // namespace scapms::utils
