
add_definitions(-DXAVNA_MOCK_UI_LIBRARY)

include_directories(${Qt5Charts_INCLUDE_DIRS})

# Instruct CMake to run moc automatically when needed
set(CMAKE_AUTOMOC ON)
# Create code from a list of Qt designer ui files
set(CMAKE_AUTOUIC ON)

set(xavna_mock_ui_SRCS
    xavna_mock_ui.C
    xavna_mock.C
    xavna_mock_ui_dialog.C)

set(xavna_mock_ui_FRMS
    xavna_mock_ui_dialog.ui)

set(xavna_mock_ui_HDRS
    xavna_mock_ui.H
    xavna_mock_ui_dialog.H
    xavna_mock_ui_global.h
    ../include/calibration.H)

add_library(xavna_mock_ui SHARED ${xavna_mock_ui_SRCS} ${xavna_mock_ui_FRMS} ${xavna_mock_ui_HDRS})

set_target_properties(xavna_mock_ui PROPERTIES
                          VERSION ${xaVNA_VERSION_STRING}
                          SOVERSION ${xaVNA_VERSION_MAJOR})

target_link_libraries(xavna_mock_ui Qt5::Core Qt5::Widgets Qt5::Gui xavna)

# Declare destinations
install(TARGETS xavna_mock_ui
        LIBRARY DESTINATION ${LIB_INSTALL_DIR}
        ARCHIVE DESTINATION ${LIB_INSTALL_DIR})
