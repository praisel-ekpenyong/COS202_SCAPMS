#pragma once
#include "model/Appointable.h"
#include "model/PatientRecord.h"

#include <string>

namespace scapms::model {

class EmergencyPatient final : public PatientRecord, public Appointable {
public:
    EmergencyPatient(std::string id, std::string name, int age,
                     std::string contact);

    std::string describe() const override;
    int priority() const override;

    void scheduleAppointment(const std::string& appointmentId) override;
    void cancelAppointment() override;
    bool isScheduled() const override;
    const std::string& currentAppointmentId() const override;

private:
    std::string scheduledAppointmentId_;
};

}  // namespace scapms::model
