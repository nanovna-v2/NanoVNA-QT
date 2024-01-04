include_directories(${Qt6Charts_INCLUDE_DIRS})

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

set(vna_diagtool_SRCS
	main.cpp
	mainwindow.C
)

set(vna_diagtool_FRMS
	mainwindow.ui
)

set(vna_diagtool_HDRS
	mainwindow.H
)

add_executable(vna_diagtool ${vna_diagtool_SRCS}
	${vna_diagtool_FRMS} ${vna_diagtool_HDRS})

target_link_libraries(vna_diagtool Qt6::Charts xavna xavna_mock_ui)

