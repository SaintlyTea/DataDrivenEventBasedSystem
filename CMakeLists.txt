cmake_minimum_required(VERSION 3.20)
project(DATADRIVENEVENTBASEDSYSTEM)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch Catch2
include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.1
)
FetchContent_MakeAvailable(Catch2)

add_executable(data_driven_event_based_system
    src/main.cpp
    src/Scripts/Models/Events/EventBase.cpp
    src/Scripts/Models/EventContext/EventContext.cpp
    src/Scripts/Models/Events/CombatEvents/EventCombatBase.cpp
    src/Scripts/Models/Expression.cpp
    src/Scripts/Models/EventOpQueue.cpp
    src/Scripts/States/ExpressionStore.cpp
    src/Scripts/Handlers/EventQueueProcessor.cpp)

add_executable(tests
    src/Scripts/Models/Events/EventBase.cpp
    src/Tests/ExpressionTests.cpp
    src/Tests/EventOpQueueTests.cpp
    src/Tests/ContextPayloadTests.cpp
    src/Tests/EventQueueProcessorTests.cpp
    src/Scripts/Models/EventContext/EventContext.cpp
    src/Scripts/Models/Expression.cpp
    src/Scripts/Models/EventOpQueue.cpp
    src/Scripts/States/ExpressionStore.cpp
    src/Scripts/Handlers/EventQueueProcessor.cpp)

target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)
target_include_directories(tests PRIVATE src)
target_include_directories(data_driven_event_based_system PRIVATE src)