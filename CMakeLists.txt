cmake_minimum_required(VERSION 3.15)
project(ftp_lite_Studio LANGUAGES CXX)

# ---- C++ configuration ----
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Enable Qt auto features ----
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ---- Qt6 setup ----
if (WIN32)
    set(Qt6_DIR "C:/Qt/6.9.3/msvc2022_64/lib/cmake/Qt6")
endif()
find_package(Qt6 COMPONENTS Widgets REQUIRED)

# ---- Include directories ----
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/third_party/sqlite3
    ${CMAKE_SOURCE_DIR}/third_party/zlib
)


file(GLOB_RECURSE RESOURCE_FILES
    ${CMAKE_SOURCE_DIR}/resources/*.qrc
)
set(SQLITE_SRC ${CMAKE_SOURCE_DIR}/third_party/sqlite3/sqlite3.c)
set_source_files_properties(${SQLITE_SRC} PROPERTIES LANGUAGE C)

add_library(sqlite3 STATIC ${SQLITE_SRC})
target_include_directories(sqlite3 PUBLIC ${CMAKE_SOURCE_DIR}/third_party/sqlite3)
set(ZLIB_INCLUDE_DIR "C:/zlib/include")
set(ZLIB_LIBRARY "C:/zlib/zlib.lib")
find_package(ZLIB REQUIRED)

# ===== SERVER EXECUTABLE =====

# ---- Core sources shared by server and client ----
file(GLOB_RECURSE SERVER_CORE_SOURCES
    ${CMAKE_SOURCE_DIR}/src/main_server.cpp
	${CMAKE_SOURCE_DIR}/src/MetadataManager.cpp
	${CMAKE_SOURCE_DIR}/src/ServerApp.cpp
	${CMAKE_SOURCE_DIR}/src/FileTransferEngine.cpp
	${CMAKE_SOURCE_DIR}/src/Logger.cpp
	${CMAKE_SOURCE_DIR}/src/CompressionHelper.cpp
	${CMAKE_SOURCE_DIR}/src/CommandParser.cpp
    ${CMAKE_SOURCE_DIR}/include/*.hpp
)

file(GLOB_RECURSE SERVER_GUI_SOURCES
    ${CMAKE_SOURCE_DIR}/gui/ServerWindow.cpp
	${CMAKE_SOURCE_DIR}/gui/ServerWindow.hpp   # Add header
)
set(SERVER_UI_FILES
    ${CMAKE_SOURCE_DIR}/gui/ServerWindow.ui
)

add_executable(ftp_lite_server
    ${SERVER_CORE_SOURCES}
    ${SERVER_GUI_SOURCES}
    ${SERVER_UI_FILES}
    ${RESOURCE_FILES}
    ${CMAKE_SOURCE_DIR}/src/main_server.cpp
)

target_include_directories(ftp_lite_server PRIVATE
	 ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/gui  # Needed to find ui_ServerWindow.h
)

target_link_libraries(ftp_lite_server PRIVATE Qt6::Widgets ws2_32 sqlite3)
target_link_libraries(ftp_lite_server PRIVATE ZLIB::ZLIB)

set_target_properties(ftp_lite_server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/server
)

# Copy server config folder
add_custom_command(TARGET ftp_lite_server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/config $<TARGET_FILE_DIR:ftp_lite_server>/config
)

# ===== CLIENT EXECUTABLE =====
# ---- Core sources shared by server and client ----
file(GLOB_RECURSE CLIENT_CORE_SOURCES
    ${CMAKE_SOURCE_DIR}/src/main_client.cpp
	${CMAKE_SOURCE_DIR}/src/ClientApp.cpp
	${CMAKE_SOURCE_DIR}/src/FileTransferEngine.cpp
	${CMAKE_SOURCE_DIR}/src/Logger.cpp
	${CMAKE_SOURCE_DIR}/src/CompressionHelper.cpp
	${CMAKE_SOURCE_DIR}/src/CommandParser.cpp
    ${CMAKE_SOURCE_DIR}/include/*.hpp
)


file(GLOB_RECURSE CLIENT_GUI_SOURCES
    ${CMAKE_SOURCE_DIR}/gui/ClientWindow.cpp
	${CMAKE_SOURCE_DIR}/gui/ClientWindow.hpp   # Add header
)
file(GLOB_RECURSE CLIENT_UI_FILES
    ${CMAKE_SOURCE_DIR}/gui/ClientWindow.ui
)

add_executable(ftp_lite_client
    ${CLIENT_CORE_SOURCES}
    ${CLIENT_GUI_SOURCES}
    ${CLIENT_UI_FILES}
    ${RESOURCE_FILES}
    ${CMAKE_SOURCE_DIR}/src/main_client.cpp
)

target_include_directories(ftp_lite_client PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/gui  # Needed to find ui_ClientWindow.h
	
)

target_link_libraries(ftp_lite_client PRIVATE Qt6::Widgets ws2_32)
target_link_libraries(ftp_lite_client PRIVATE ZLIB::ZLIB)


set_target_properties(ftp_lite_client PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/client
)

# Copy client config folder
add_custom_command(TARGET ftp_lite_client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/config $<TARGET_FILE_DIR:ftp_lite_client>/config
)

# ===== Source groups for Visual Studio =====
source_group("Core Sources" FILES ${CORE_SOURCES})
source_group("Server GUI" FILES ${SERVER_GUI_SOURCES} ${SERVER_UI_FILES})
source_group("Client GUI" FILES ${CLIENT_GUI_SOURCES} ${CLIENT_UI_FILES})
source_group("Resources" FILES ${RESOURCE_FILES})

# ===== Status messages =====
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Qt6 found at: ${Qt6_DIR}")
