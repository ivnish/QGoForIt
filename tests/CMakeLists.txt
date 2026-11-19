find_package(Qt6 REQUIRED COMPONENTS Test Widgets)

set(QGOFORIT_TEST_COMMON_SOURCES
    ../src/utils.cpp
    ../src/baserowwidget.cpp
)

set(QGOFORIT_TESTS test_fileutils test_taskstorage)

foreach(_test ${QGOFORIT_TESTS})
    qt_add_executable(${_test} ${_test}.cpp)

    if(_test STREQUAL test_taskstorage)
        target_sources(${_test} PRIVATE
            ../src/todowidget.cpp
            ../src/filemonitor.cpp
        )
    endif()

    target_sources(${_test} PRIVATE ${QGOFORIT_TEST_COMMON_SOURCES})

    target_include_directories(${_test} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
        ${CMAKE_CURRENT_BINARY_DIR}/../generated
    )

    target_link_libraries(${_test} PRIVATE Qt6::Test Qt6::Widgets)

    add_test(NAME ${_test} COMMAND ${_test})
    set_tests_properties(${_test} PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endforeach()
