#pragma once
// Generates sequential IDs for patients (P-xxxx) and appointments (A-xxxx).
#include <string>

namespace scapms::utils {

class IDGenerator {
public:
    // Issues the next patient ID: P-0001, then P-0002, and so on.
    std::string nextPatientId();

    // Issues the next appointment ID: A-0001, then A-0002, and so on.
    std::string nextAppointmentId();

    // Current patient counter
    unsigned patientCounter() const;

    // Current appointment counter
    unsigned appointmentCounter() const;

    // Restores counters from loaded file data.
    void restorePatientCounter(unsigned highestIssued);
    void restoreAppointmentCounter(unsigned highestIssued);

private:
    unsigned patientCounter_ = 0;
    unsigned appointmentCounter_ = 0;
};

// Validates ID format ("P-0001" or "A-0001").
bool isPatientId(const std::string& s);
bool isAppointmentId(const std::string& s);

}  // namespace scapms::utils
