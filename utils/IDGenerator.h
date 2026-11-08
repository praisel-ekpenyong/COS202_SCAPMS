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

    // Highest patient number issued so far; 0 means none yet.
    unsigned patientCounter() const;

    // Highest appointment number issued so far; 0 means none yet.
    unsigned appointmentCounter() const;

    // Restores a persisted counter (the highest number ever issued). The next
    // ID is counter + 1, so IDs from a loaded file are never reissued.
    void restorePatientCounter(unsigned highestIssued);
    void restoreAppointmentCounter(unsigned highestIssued);

private:
    unsigned patientCounter_ = 0;
    unsigned appointmentCounter_ = 0;
};

// True iff s matches the exact ID shapes "P-0001" / "A-0001" (four or more
// digits after the prefix). Used to validate IDs read from files or the GUI.
bool isPatientId(const std::string& s);
bool isAppointmentId(const std::string& s);

}  // namespace scapms::utils
