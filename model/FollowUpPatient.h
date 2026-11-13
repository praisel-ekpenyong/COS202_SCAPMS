#pragma once
#include "model/Appointable.h"
#include "model/PatientRecord.h"

#include <string>

namespace scapms::model {

class FollowUpPatient final : public PatientRecord, public Appointable {
public:
    // nextVisitDate is "YYYY-MM-DD" or empty if none.
    FollowUpPatient(std::string id, std::string name, int age,
                    std::string contact, const std::string& nextVisitDate = "");

    std::string describe() const override;
    int priority() const override;

    // Returns next-visit date, or empty string if not set.
    const std::string& nextVisitDate() const;

    // Updates the next-visit date.
    void setNextVisitDate(const std::string& nextVisitDate);

    void scheduleAppointment(const std::string& appointmentId) override;
    void cancelAppointment() override;
    bool isScheduled() const override;
    const std::string& currentAppointmentId() const override;

private:
    std::string nextVisitDate_;
    std::string scheduledAppointmentId_;
};

}  // namespace scapms::model
