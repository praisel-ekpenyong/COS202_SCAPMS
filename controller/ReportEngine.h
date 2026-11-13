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
    // Most attended patients report
    struct AttendedRow {
        std::string patientId;
        std::string name;
        model::PatientCategory category{};
        int completedCount = 0;
    };

    // Pending appointments report
    struct PendingRow {
        std::string appointmentId;
        std::string patientId;
        std::string patientName;
        std::string date;  // "YYYY-MM-DD"
        std::string time;  // "HH:MM"
        bool overdue = false;
    };

    // Category distribution report
    struct CategoryRow {
        model::PatientCategory category{};
        int count = 0;
        double sharePercent = 0.0;
    };

    // Daily appointment summary report
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

    // Counts and percentages for each patient category.
    std::vector<CategoryRow> categoryDistribution() const;

    // Appointments dated `date` split by status.
    DailySummary dailySummary(const std::string& date) const;

    // Plain text formatters for export
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
