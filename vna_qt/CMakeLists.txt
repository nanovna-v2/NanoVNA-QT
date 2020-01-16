
include_directories(${Qt5Charts_INCLUDE_DIRS})

# Instruct CMake to run moc automatically when needed
set(CMAKE_AUTOMOC ON)
# Create code from a list of Qt designer ui files
set(CMAKE_AUTOUIC ON)

set(vna_qt_SRCS
    polarview.C
    mainwindow.C
    main.C
    markerslider.C
    impedancedisplay.C
    frequencydialog.C
    graphpanel.C
    configureviewdialog.C
    touchstone.C
    calkitsettingsdialog.C
    calkitsettings.C
    networkview.C
    dtfwindow.C)

set(vna_qt_FRMS
    mainwindow.ui
    markerslider.ui
    impedancedisplay.ui
    frequencydialog.ui
    graphpanel.ui
    configureviewdialog.ui
    calkitsettingsdialog.ui
    calkitsettingswidget.ui
    dtfwindow.ui
    graphlimitsdialog.ui)

set(vna_qt_HDRS
    polarview.H
    mainwindow.H
    markerslider.H
    impedancedisplay.H
    utility.H
    frequencydialog.H
    graphpanel.H
    configureviewdialog.H
    touchstone.H
    calkitsettingsdialog.H
    calkitsettings.H
    networkview.H
    dtfwindow.H)


add_executable(vna_qt ${vna_qt_SRCS} ${vna_qt_FRMS} ${vna_qt_HDRS})

target_link_libraries(vna_qt Qt5::Charts ${FFTW3_LIBRARIES} xavna xavna_mock_ui)

# Install destinations
install(TARGETS vna_qt RUNTIME DESTINATION ${BIN_INSTALL_DIR})
