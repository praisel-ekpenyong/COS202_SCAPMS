#pragma once
// Generates the four clinic analytical reports: most attended patients,
// pending appointments, category distribution, and daily appointment summary.
#include "controller/SortAlgorithms.h"
#include "model/Appointment.h"
#include "model/AppointmentStatus.h"
#include "model/ClinicDatabase.h"
#include "model/PatientCategory.h"
#include "model/PatientRecord.h"

#include <cstddef>
#include <string>
#include <vector>

namespace scapms::controller {

class ReportEngine {
public:
    // --- report 1: most frequently attended patients ---

    // One ranked patient: how many of their appointments were completed.
    struct AttendedRow {
        std::string patientId;
        std::string name;
        model::PatientCategory category{};
        int completedCount = 0;
    };

    // --- report 2: pending appointments ---

    // One pending appointment with its patient's name resolved for display.
    // "Overdue" is the definition: pending and dated before `today`.
    struct PendingRow {
        std::string appointmentId;
        std::string patientId;
        std::string patientName;
        std::string date;  // "YYYY-MM-DD"
        std::string time;  // "HH:MM"
        bool overdue = false;
    };

    // --- report 3: category distribution ---

    // One row per category, in the fixed General / Emergency / Follow-Up
    // order, so zero-count categories stay visible instead of vanishing.
    struct CategoryRow {
        model::PatientCategory category{};
        int count = 0;
        double sharePercent = 0.0;  // count * 100 / all patients; 0 when empty
    };

    // --- report 4: daily appointment summary ---

    // Totals for one day. A day with no appointments reports all zeros
    // rather than "nothing" - staff still want the confirmation.
    struct DailySummary {
        std::string date;
        int total = 0;
        int completed = 0;
        int pending = 0;
        int cancelled = 0;
    };

    // ReportEngine queries the database to compute report rows.
    explicit ReportEngine(const model::ClinicDatabase& database);

    // Top attended patients ranked by completed appointments.
    std::vector<AttendedRow> mostAttended(std::size_t limit = 5) const;

    // Pending appointments ordered by date and time.
    std::vector<PendingRow> pendingAppointments(const std::string& today) const;

    // One row per category (fixed order above) with counts and shares.
    std::vector<CategoryRow> categoryDistribution() const;

    // Appointments dated `date` split by status.
    DailySummary dailySummary(const std::string& date) const;

    // --- plain-text formatters for file export ---

    // Formats report tables as aligned plain text.
    static std::string mostAttendedText(const std::vector<AttendedRow>& rows);
    static std::string pendingAppointmentsText(const std::vector<PendingRow>& rows);
    static std::string categoryDistributionText(const std::vector<CategoryRow>& rows);
    static std::string dailySummaryText(const DailySummary& summary);

    // Formats percentage with one decimal place.
    static std::string sharePercentText(double sharePercent);

private:
    const model::ClinicDatabase& database_;
};

}  // namespace scapms::controller
