cmake_minimum_required(VERSION 3.10)
project(telegram_bot_cpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(CURL REQUIRED)
find_package(nlohmann_json 3.10.0 QUIET)

# Check if the file exists
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/include/nlohmann/json.hpp)
    # Create directory if it doesn't exist
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/nlohmann)

    # Download the file
    message(STATUS "Downloading nlohmann/json.hpp...")
    file(DOWNLOAD
            https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/include/nlohmann/json.hpp
            SHOW_PROGRESS
            STATUS download_status
    )

    # Check download status
    list(GET download_status 0 status_code)
    if(NOT status_code EQUAL 0)
        message(FATAL_ERROR "Failed to download nlohmann/json.hpp")
    endif()
endif()

# Include directories
include_directories(${CURL_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Add executable
add_executable(telegram_bot
    main.cpp
    Models/TelegramBot.cpp
)

# Link libraries
target_link_libraries(telegram_bot PRIVATE ${CURL_LIBRARIES})

# Copy the environment file to the build directory
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/dev.env
        ${CMAKE_CURRENT_BINARY_DIR}/dev.env COPYONLY)