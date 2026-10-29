cmake_minimum_required(VERSION 3.19)
project(DaySequence VERSION 2.3.0 LANGUAGES CXX)
set(CMAKE_INSTALL_BINDIR ".")
set(CMAKE_INSTALL_LIBDIR ".")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(Qt6 6.8 REQUIRED COMPONENTS Core Widgets Network)
qt_standard_project_setup()

qt_add_library(daysequence_data STATIC
    src/core/planner.cpp src/core/planner.h
    src/core/countdown.cpp src/core/countdown.h
    src/services/weatherdata.cpp src/services/weatherdata.h
    src/services/storage.cpp src/services/storage.h
)
target_include_directories(daysequence_data PUBLIC src/core src/services)
target_link_libraries(daysequence_data PUBLIC Qt6::Core)
qt_add_library(daysequence_ui STATIC
    src/ui/mainwindow.cpp src/ui/mainwindow.h src/ui/appearance.cpp src/ui/preferences.cpp src/ui/weather.cpp
    src/ui/widgets/visuals.cpp src/ui/widgets/visuals.h
    src/ui/widgets/focusclock.cpp src/ui/widgets/focusclock.h
    src/ui/widgets/tasklist.cpp src/ui/widgets/tasklist.h
    src/ui/widgets/choicebox.cpp src/ui/widgets/choicebox.h src/ui/widgets/pathlabel.h
    src/ui/components/icons.cpp src/ui/components/icons.h src/ui/components/uicomponents.h
)
target_include_directories(daysequence_ui PUBLIC src/ui src/ui/widgets src/ui/components)
qt_add_resources(daysequence_ui icons PREFIX "/" BASE resources FILES resources/check.svg resources/app.svg)
target_link_libraries(daysequence_ui PUBLIC daysequence_data Qt6::Widgets Qt6::Network)
qt_add_executable(DaySequence WIN32 src/app/main.cpp)
target_compile_definitions(DaySequence PRIVATE DAYSEQUENCE_VERSION="${PROJECT_VERSION}")
target_link_libraries(DaySequence PRIVATE daysequence_ui)
if(WIN32)
    enable_language(RC)
    configure_file(resources/windows.rc.in "${CMAKE_CURRENT_BINARY_DIR}/DaySequence.rc" @ONLY)
    target_sources(DaySequence PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/DaySequence.rc")
endif()
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
    qt_add_executable(planner_tests tests/tests.cpp)
    target_link_libraries(planner_tests PRIVATE daysequence_data Qt6::Test)
    add_test(NAME planner_tests COMMAND planner_tests)
    qt_add_executable(ui_tests tests/uitests.cpp)
    target_link_libraries(ui_tests PRIVATE daysequence_ui Qt6::Test)
    add_test(NAME ui_tests COMMAND ui_tests -platform offscreen)
    qt_add_executable(storage_tests tests/storage_tests.cpp)
    target_link_libraries(storage_tests PRIVATE daysequence_data Qt6::Test)
    add_test(NAME storage_tests COMMAND storage_tests)
    qt_add_executable(lifecycle_tests tests/lifecycle_tests.cpp)
    target_link_libraries(lifecycle_tests PRIVATE daysequence_ui Qt6::Test)
    add_test(NAME lifecycle_tests COMMAND lifecycle_tests -platform offscreen)
    set_tests_properties(planner_tests ui_tests storage_tests lifecycle_tests PROPERTIES TIMEOUT 30)
endif()
install(TARGETS DaySequence RUNTIME DESTINATION .)
qt_generate_deploy_app_script(TARGET DaySequence OUTPUT_SCRIPT deploy_script NO_UNSUPPORTED_PLATFORM_ERROR NO_TRANSLATIONS DEPLOY_TOOL_OPTIONS --no-opengl-sw --no-system-d3d-compiler)
install(SCRIPT ${deploy_script})
install(FILES LICENSE NOTICE THIRD_PARTY_NOTICES.md RELEASE_NOTES.md DESTINATION .)
install(DIRECTORY licenses/ DESTINATION licenses)
