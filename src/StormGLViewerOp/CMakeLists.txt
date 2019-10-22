set(PLUGIN_NAME StormGLViewer)

add_library(${PLUGIN_NAME} SHARED
    StormGLViewer.cpp)

target_include_directories(${PLUGIN_NAME}
    PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
    ${NUKE_INCLUDE_DIRS}
    ${USD_ROOT}/include)

target_link_libraries(${PLUGIN_NAME}
    ${HDNUKE_LIB_NAME}
    ${NUKE_DDIMAGE_LIBRARY}
    arch hd hdx usdImaging usdImagingGL)

set_target_properties(${PLUGIN_NAME}
    PROPERTIES
    PREFIX ""
    INSTALL_RPATH "$ORIGIN/../lib"
    INSTALL_RPATH_USE_LINK_PATH True)

install(TARGETS ${PLUGIN_NAME}
    DESTINATION plugins)
