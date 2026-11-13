# Static library: scapms::utils - shared helpers (app identity, file I/O, exceptions).
add_library(scapms_utils STATIC
    AppInfo.cpp
    AppInfo.h
    Exceptions.h
    FileHandler.cpp
    FileHandler.h
    IDGenerator.cpp
    IDGenerator.h
    Validators.cpp
    Validators.h
)
target_include_directories(scapms_utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
