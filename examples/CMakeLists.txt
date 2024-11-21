# Add all example subdirectories
file(GLOB EXAMPLE_DIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)

foreach(EXAMPLE_DIR ${EXAMPLE_DIRS})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${EXAMPLE_DIR})
        add_subdirectory(${EXAMPLE_DIR})
        message(STATUS "Building example application: ${EXAMPLE_DIR}")
    endif()
endforeach()
