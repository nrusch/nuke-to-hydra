add_library(${HDNUKE_LIB_NAME} SHARED
    geoAdapter.cpp
    lightAdapter.cpp
    renderStack.cpp
    sceneDelegate.cpp
    tokens.cpp)

target_include_directories(${HDNUKE_LIB_NAME}
    PRIVATE
    ${NUKE_INCLUDE_DIRS}
    ${USD_ROOT}/include)

target_link_libraries(${HDNUKE_LIB_NAME}
    ${NUKE_DDIMAGE_LIBRARY}
    arch hd)

set_target_properties(${HDNUKE_LIB_NAME}
    PROPERTIES
    INSTALL_RPATH_USE_LINK_PATH True)

install(TARGETS ${HDNUKE_LIB_NAME}
    DESTINATION lib)
