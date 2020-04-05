
set(libxavna_SRC xavna.C
                 xavna_cpp.C
                 calibration.C)

if(WIN32)
  list(APPEND libxavna_SRC platform_abstraction_windows.C)
else()
  list(APPEND libxavna_SRC platform_abstraction.C)
endif()

add_library(xavna SHARED ${libxavna_SRC})

set_target_properties(xavna PROPERTIES
                        VERSION ${xaVNA_VERSION_STRING}
                        SOVERSION ${xaVNA_VERSION_MAJOR})

target_link_libraries(xavna ${xa_LIBRARIES})

# Declare destinations
install(TARGETS xavna
        LIBRARY DESTINATION ${LIB_INSTALL_DIR}
        ARCHIVE DESTINATION ${LIB_INSTALL_DIR})
