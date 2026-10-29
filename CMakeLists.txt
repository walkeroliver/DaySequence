cmake_minimum_required(VERSION 3.19)
project(DaySequence VERSION 2.1.2 LANGUAGES CXX)
set(CMAKE_INSTALL_BINDIR ".")
set(CMAKE_INSTALL_LIBDIR ".")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(Qt6 6.8 REQUIRED COMPONENTS Core Widgets Network)
qt_standard_project_setup()

qt_add_library(daysequence_data STATIC planner.cpp planner.h weatherdata.cpp weatherdata.h)
target_link_libraries(daysequence_data PUBLIC Qt6::Core)
qt_add_library(daysequence_ui STATIC
    mainwindow.cpp mainwindow.h appearance.cpp preferences.cpp weather.cpp
    visuals.cpp visuals.h tasklist.cpp tasklist.h icons.cpp icons.h uicomponents.h
)
qt_add_resources(daysequence_ui icons PREFIX "/" FILES check.svg)
target_link_libraries(daysequence_ui PUBLIC daysequence_data Qt6::Widgets Qt6::Network)
qt_add_executable(DaySequence WIN32 main.cpp)
target_link_libraries(DaySequence PRIVATE daysequence_ui)

foreach(target daysequence_data daysequence_ui DaySequence)
    if(MSVC)
        target_compile_options(${target} PRIVATE /utf-8 /W4)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endforeach()

include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    qt_add_executable(planner_tests tests.cpp)
    target_link_libraries(planner_tests PRIVATE daysequence_data Qt6::Test)
    add_test(NAME planner_tests COMMAND planner_tests)
    qt_add_executable(ui_tests uitests.cpp)
    target_link_libraries(ui_tests PRIVATE daysequence_ui Qt6::Test)
    add_test(NAME ui_tests COMMAND ui_tests -platform offscreen)
    set_tests_properties(planner_tests ui_tests PROPERTIES TIMEOUT 30)
endif()
install(TARGETS DaySequence RUNTIME DESTINATION .)
qt_generate_deploy_app_script(
    TARGET DaySequence
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
    NO_TRANSLATIONS
    DEPLOY_TOOL_OPTIONS --no-opengl-sw --no-system-d3d-compiler
)
install(SCRIPT ${deploy_script})

install(FILES LICENSE NOTICE THIRD_PARTY_NOTICES.md RELEASE_NOTES.md DESTINATION .)
install(DIRECTORY licenses/ DESTINATION licenses)
