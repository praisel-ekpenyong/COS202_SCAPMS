# Static library: scapms::model - domain records and storage.
add_library(scapms_model STATIC
    Appointment.cpp
    Appointment.h
    AppointmentStatus.h
    Appointable.h
    ClinicDatabase.cpp
    ClinicDatabase.h
    EmergencyPatient.cpp
    EmergencyPatient.h
    FollowUpPatient.cpp
    FollowUpPatient.h
    GeneralPatient.cpp
    GeneralPatient.h
    PatientCategory.h
    PatientRecord.cpp
    PatientRecord.h
)
target_include_directories(scapms_model PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(scapms_model PUBLIC scapms_utils)
