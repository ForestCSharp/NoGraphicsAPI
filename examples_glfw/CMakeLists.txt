set(NOGRAPHICSAPI_GLFW_VERSION 3.5.1)
set(NOGRAPHICSAPI_GLFW_DIR ${CMAKE_CURRENT_SOURCE_DIR}/glfw)
set(NOGRAPHICSAPI_GLFW_INCLUDE_DIR
    ${NOGRAPHICSAPI_GLFW_DIR}/glfw-${NOGRAPHICSAPI_GLFW_VERSION}/include
    CACHE PATH "GLFW include directory")

if(NOT EXISTS ${NOGRAPHICSAPI_GLFW_INCLUDE_DIR}/GLFW/glfw3.h)
    message(STATUS "Downloading GLFW ${NOGRAPHICSAPI_GLFW_VERSION} headers")
    file(MAKE_DIRECTORY ${NOGRAPHICSAPI_GLFW_DIR})
    set(glfw_archive ${NOGRAPHICSAPI_GLFW_DIR}/glfw-${NOGRAPHICSAPI_GLFW_VERSION}.zip)
    file(DOWNLOAD
        https://github.com/glfw/glfw/releases/download/${NOGRAPHICSAPI_GLFW_VERSION}/glfw-${NOGRAPHICSAPI_GLFW_VERSION}.zip
        ${glfw_archive}
        EXPECTED_HASH SHA256=ea79bc5feffc254c87291980c2d0bce9acebb68c4983b79f961dcd2cb8a611a0
    )
    file(ARCHIVE_EXTRACT INPUT ${glfw_archive} DESTINATION ${NOGRAPHICSAPI_GLFW_DIR})
    file(REMOVE ${glfw_archive})
endif()

find_library(NOGRAPHICSAPI_GLFW_LIBRARY NAMES glfw glfw3 libglfw.so.3 REQUIRED)

include(${PROJECT_SOURCE_DIR}/examples/NoGraphicsAPI_slang.cmake)

add_library(NoGraphicsAPI_glfw INTERFACE)
target_include_directories(NoGraphicsAPI_glfw INTERFACE ${NOGRAPHICSAPI_GLFW_INCLUDE_DIR})
target_link_libraries(NoGraphicsAPI_glfw INTERFACE
    NoGraphicsAPI::NoGraphicsAPI
    ${NOGRAPHICSAPI_GLFW_LIBRARY}
)

function(NoGraphicsAPI_add_glfw_example target)
    add_executable(${target} ${ARGN})
    foreach(source IN LISTS ARGN)
        if(source MATCHES "\\.slang$")
            set_source_files_properties(${source} PROPERTIES HEADER_FILE_ONLY TRUE)
            source_group("Shaders" FILES ${source})
        endif()
    endforeach()
    NoGraphicsAPI_disable_exceptions(${target})
    target_link_libraries(${target} PRIVATE NoGraphicsAPI_glfw)
endfunction()

add_subdirectory(triangle)
add_subdirectory(cube)
add_subdirectory(deferred_renderer)
