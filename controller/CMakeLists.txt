# Static library: scapms::controller - business logic.
add_library(scapms_controller STATIC
    ClinicManager.cpp
    ClinicManager.h
    UndoableAction.cpp
    UndoableAction.h
    PatientSorter.cpp
    PatientSorter.h
    ReportEngine.cpp
    ReportEngine.h
    SearchEngine.cpp
    SearchEngine.h
)
target_include_directories(scapms_controller PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(scapms_controller PUBLIC scapms_model scapms_utils)
