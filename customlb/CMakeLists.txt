cmake_minimum_required(VERSION 3.10)
project(CustomLoadBalancer)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2 -pthread")

# Source files
set(SOURCES
    main_new.cpp
    LoadBalancer.cpp
)

# Headers
set(HEADERS
    LoadBalancer.h
)

# Create executable
add_executable(loadbalancer ${SOURCES} ${HEADERS})

# Link pthread library
target_link_libraries(loadbalancer pthread)

# Install target
install(TARGETS loadbalancer DESTINATION /usr/local/bin)
