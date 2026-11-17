cmake_minimum_required(VERSION 4.0)
project(lab2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(lab2 main.cpp
        BinaryHeap.h)
add_executable(knapsack_tests tests/knapsack_tests.cpp)
add_executable(data_structure_tests tests/data_structure_tests.cpp)
add_executable(knapsack_benchmarks tests/knapsack_benchmarks.cpp)
add_executable(lab2_web_backend gui/WebBackend.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

if(APPLE)
    enable_language(OBJCXX)
    set(CMAKE_OBJCXX_STANDARD 20)
    set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
    add_executable(lab2_gui MACOSX_BUNDLE gui/KnapsackApp.mm gui/Knapsack.icns)
    set_source_files_properties(gui/Knapsack.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    target_compile_options(lab2_gui PRIVATE -fobjc-arc)
    target_link_libraries(lab2_gui PRIVATE "-framework Cocoa" "-framework WebKit")
    set_target_properties(lab2_gui PROPERTIES
            MACOSX_BUNDLE_BUNDLE_NAME "Рюкзак"
            MACOSX_BUNDLE_GUI_IDENTIFIER "ru.lab2.knapsack"
            MACOSX_BUNDLE_ICON_FILE "Knapsack.icns"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
            MACOSX_BUNDLE_BUNDLE_VERSION "1")
    add_custom_command(TARGET lab2_gui POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/web
            $<TARGET_BUNDLE_CONTENT_DIR:lab2_gui>/Resources/web)
    set_property(TARGET lab2_gui APPEND PROPERTY LINK_DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/web/index.html
            ${CMAKE_CURRENT_SOURCE_DIR}/web/style.css
            ${CMAKE_CURRENT_SOURCE_DIR}/web/search.js
            ${CMAKE_CURRENT_SOURCE_DIR}/web/structures.js
            ${CMAKE_CURRENT_SOURCE_DIR}/web/app.js)
endif()

if(MSVC)
    target_compile_options(lab2_web_backend PRIVATE /O2 /utf-8)
else()
    target_compile_options(lab2_web_backend PRIVATE -O2)
endif()

find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_FOUND)
    add_custom_target(lab2_web
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/server.py
                    --backend $<TARGET_FILE:lab2_web_backend>
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            DEPENDS lab2_web_backend
            USES_TERMINAL)
endif()

foreach(target knapsack_tests data_structure_tests)
    if(MSVC)
        target_compile_options(${target} PRIVATE /UNDEBUG)
    else()
        target_compile_options(${target} PRIVATE
                -g -UNDEBUG -fsanitize=address,undefined
                -fno-omit-frame-pointer -fno-sanitize-recover=all)
        target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endif()
endforeach()

enable_testing()
add_test(NAME knapsack_correctness COMMAND knapsack_tests)
add_test(NAME data_structures_correctness COMMAND data_structure_tests)
set_tests_properties(knapsack_correctness data_structures_correctness PROPERTIES TIMEOUT 60)
if(Python3_FOUND)
    add_test(NAME web_backend_correctness
            COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/tests/web_tests.py
                    $<TARGET_FILE:lab2_web_backend>)
    set_tests_properties(web_backend_correctness PROPERTIES TIMEOUT 60)
endif()

add_custom_target(check
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS knapsack_tests data_structure_tests lab2_web_backend
        USES_TERMINAL)

if(APPLE)
    add_executable(knapsack_tests_leaks tests/knapsack_tests.cpp)
    add_executable(data_structure_tests_leaks tests/data_structure_tests.cpp)
    foreach(target knapsack_tests_leaks data_structure_tests_leaks)
        target_compile_options(${target} PRIVATE -g -O0 -UNDEBUG)
    endforeach()

    add_custom_target(check_leaks
            COMMAND ${CMAKE_COMMAND} -E env MallocStackLogging=1
                    /usr/bin/leaks --atExit -- $<TARGET_FILE:knapsack_tests_leaks>
            COMMAND ${CMAKE_COMMAND} -E env MallocStackLogging=1
                    /usr/bin/leaks --atExit -- $<TARGET_FILE:data_structure_tests_leaks>
            DEPENDS knapsack_tests_leaks data_structure_tests_leaks
            USES_TERMINAL)
endif()
