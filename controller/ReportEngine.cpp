#include "controller/ReportEngine.h"

#include <map>

namespace scapms::controller {

namespace {

// Left-aligns text to width
std::string padTo(const std::string& text, std::size_t width) {
    if (text.size() >= width) return text;
    return text + std::string(width - text.size(), ' ');
}

// Right-aligns text to width
std::string padLeft(const std::string& text, std::size_t width) {
    if (text.size() >= width) return text;
    return std::string(width - text.size(), ' ') + text;
}

}  // namespace

// Format percentage with one decimal place
std::string ReportEngine::sharePercentText(double percent) {
    if (percent < 0.0) percent = 0.0;
    const long tenths = static_cast<long>(percent * 10.0 + 0.5);
    return std::to_string(tenths / 10) + "." + std::to_string(tenths % 10);
}


ReportEngine::ReportEngine(const model::ClinicDatabase& database)
    : database_(database) {}

std::vector<ReportEngine::AttendedRow> ReportEngine::mostAttended(
    std::size_t limit) const {
    // Count completed visits per patient
    std::map<std::string, int> completedByPatient;
    for (const auto& appointment : database_.appointments()) {
        if (appointment->status() == model::AppointmentStatus::Completed) {
            ++completedByPatient[appointment->patientId()];
        }
    }

    std::vector<AttendedRow> rows;
    rows.reserve(completedByPatient.size());
    for (const auto& patient : database_.patients()) {
        const auto found = completedByPatient.find(patient->id());
        if (found == completedByPatient.end()) continue;
        rows.push_back({patient->id(), patient->name(), patient->category(),
                        found->second});
    }

    // Sort by completed appointments descending, ties broken by name
    mergeSort(rows, [](const AttendedRow& a, const AttendedRow& b) {
        if (a.completedCount != b.completedCount) {
            return a.completedCount > b.completedCount;
        }
        return a.name < b.name;
    });

    if (rows.size() > limit) rows.resize(limit);
    return rows;
}

std::vector<ReportEngine::PendingRow> ReportEngine::pendingAppointments(
    const std::string& today) const {
    std::vector<PendingRow> rows;
    for (const auto& appointment : database_.appointments()) {
        if (appointment->status() != model::AppointmentStatus::Pending) {
            continue;
        }
        const model::PatientRecord* patient =
            database_.findPatient(appointment->patientId());
        rows.push_back({appointment->id(), appointment->patientId(),
                        patient ? patient->name() : "(unknown patient)",
                        appointment->date(), appointment->time(),
                        appointment->isOverdue(today)});
    }

    // Sort by date, time, then appointment ID
    mergeSort(rows, [](const PendingRow& a, const PendingRow& b) {
        if (a.date != b.date) return a.date < b.date;
        if (a.time != b.time) return a.time < b.time;
        return a.appointmentId < b.appointmentId;
    });
    return rows;
}

std::vector<ReportEngine::CategoryRow> ReportEngine::categoryDistribution()
    const {
    int counts[3] = {0, 0, 0};
    for (const auto& patient : database_.patients()) {
        ++counts[static_cast<int>(patient->category())];
    }

    const double total = static_cast<double>(database_.patients().size());
    std::vector<CategoryRow> rows;
    rows.reserve(3);
    for (int i = 0; i < 3; ++i) {
        const auto category = static_cast<model::PatientCategory>(i);
        const double share =
            total > 0.0 ? 100.0 * static_cast<double>(counts[i]) / total : 0.0;
        rows.push_back({category, counts[i], share});
    }
    return rows;
}

ReportEngine::DailySummary ReportEngine::dailySummary(
    const std::string& date) const {
    DailySummary summary;
    summary.date = date;
    for (const auto& appointment : database_.appointments()) {
        if (appointment->date() != date) continue;
        ++summary.total;
        switch (appointment->status()) {
            case model::AppointmentStatus::Completed:
                ++summary.completed;
                break;
            case model::AppointmentStatus::Pending:
                ++summary.pending;
                break;
            case model::AppointmentStatus::Cancelled:
                ++summary.cancelled;
                break;
        }
    }
    return summary;
}

// Plain-text rendering for report export.

std::string ReportEngine::mostAttendedText(
    const std::vector<AttendedRow>& rows) {
    if (rows.empty()) {
        return "No completed appointments yet - nothing to rank.\n";
    }
    std::string text =
        "Ranked by completed appointments (ties broken by name).\n";
    text += "  #  ID       Name                       Category     Completed\n";
    int rank = 1;
    for (const auto& row : rows) {
        text += padLeft(std::to_string(rank), 2) + "  ";
        text += padTo(row.patientId, 9);
        text += padTo(row.name, 27);
        text += padTo(model::categoryName(row.category), 13);
        text += std::to_string(row.completedCount);
        text += "\n";
        ++rank;
    }
    return text;
}

std::string ReportEngine::pendingAppointmentsText(
    const std::vector<PendingRow>& rows) {
    if (rows.empty()) {
        return "No pending appointments.\n";
    }
    std::string text =
        "Pending appointments, soonest first (overdue = pending and dated "
        "before today).\n";
    text += "ID       Patient  Name                  Date        Time   "
            "Status\n";
    for (const auto& row : rows) {
        text += padTo(row.appointmentId, 9);
        text += padTo(row.patientId, 9);
        text += padTo(row.patientName, 22);
        text += padTo(row.date, 12);
        text += padTo(row.time, 7);
        text += row.overdue ? "OVERDUE" : "pending";
        text += "\n";
    }
    return text;
}

std::string ReportEngine::categoryDistributionText(
    const std::vector<CategoryRow>& rows) {
    int total = 0;
    for (const auto& row : rows) total += row.count;
    if (total == 0) {
        return "No patients registered yet - nothing to distribute.\n";
    }
    std::string text = "Patients per category (share of all " +
                       std::to_string(total) + " patients).\n";
    for (const auto& row : rows) {
        text += padTo(model::categoryName(row.category), 12);
        text += padTo(std::to_string(row.count), 6);
        text += ReportEngine::sharePercentText(row.sharePercent) + "%\n";    }
    return text;
}

std::string ReportEngine::dailySummaryText(const DailySummary& summary) {
    std::string text =
        "Appointments on " + summary.date + ": " +
        std::to_string(summary.total) + " total.\n";
    text += "  completed  " + std::to_string(summary.completed) + "\n";
    text += "  pending    " + std::to_string(summary.pending) + "\n";
    text += "  cancelled  " + std::to_string(summary.cancelled) + "\n";
    return text;
}

}  // namespace scapms::controller
