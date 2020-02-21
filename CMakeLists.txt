project(findText)
cmake_minimum_required(VERSION 3.16)

set( EXECUTABLE_OUTPUT_PATH ../bin)
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ../bin)
set(CMAKE_CXX_FLAGS_RELEASE "-s -Wl,-gc-sections -mwindows")

add_link_options("-Wl,--large-address-aware")
add_executable(findText resource.rc findcore.cc main.cc)
target_link_libraries(findText -lwin32hlp -lexshit -lstdshit -lcomdlg32 -luuid)
