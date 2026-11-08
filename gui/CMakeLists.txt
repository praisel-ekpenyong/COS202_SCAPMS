# Static library: scapms::gui - Qt Widgets UI.
set(CMAKE_AUTORCC ON)

add_library(scapms_gui STATIC
    AdminPanel.cpp
    AdminPanel.h
    AppointmentPanel.cpp
    AppointmentPanel.h
    DashboardPanel.cpp
    DashboardPanel.h
    MainWindow.cpp
    MainWindow.h
    PatientPanel.cpp
    PatientPanel.h
    QueuePanel.cpp
    QueuePanel.h
    ReportPanel.cpp
    ReportPanel.h
    SearchPanel.cpp
    SearchPanel.h
    Theme.cpp
    Theme.h
    resources.qrc
)
target_link_libraries(scapms_gui
    PUBLIC
        Qt6::Widgets
        scapms_model
        scapms_controller
        scapms_utils
)

# The application executable. WIN32 keeps it a GUI-subsystem binary (no console window).
add_executable(SCAPMS WIN32 main.cpp)
target_link_libraries(SCAPMS PRIVATE scapms_gui)
