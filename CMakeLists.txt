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
    src/Scripts/Models/EventQueue/EventQueue.cpp
    src/Scripts/States/ExpressionStore.cpp
    src/Scripts/Handlers/EventQueueHandler.cpp
    src/Scripts/States/EventStore.cpp
    src/Scripts/Models/Events/CombatEvents/Effects/ApplyStatusEffect.cpp
    src/Scripts/Models/EventContext/EventDepthTracker.cpp
    src/Scripts/Models/ContextPayload/Combat/Damage/DamageCalcData.cpp
    src/Scripts/Models/ContextPayload/Combat/Damage/DamageTakenData.cpp
    src/Scripts/Models/ContextPayload/Utility/CancellationPayload.cpp
    src/Scripts/Models/ContextPayload/Utility/BanPayload.cpp
    src/Scripts/Handlers/Targeting/TargetResolver.cpp
    src/Scripts/Handlers/EventSorter.cpp
    src/Scripts/Helpers/Context/ContextBuilder.cpp
    src/Scripts/Handlers/EventFiring.cpp
    src/Scripts/Models/EventContext/Combat/EventCombatContext.cpp
    src/Scripts/Handlers/Targeting/TargetResolver.cpp)

add_executable(tests
    src/Scripts/Models/Events/EventBase.cpp
    src/Scripts/Models/ContextPayload/Combat/Damage/DamageCalcData.cpp
    src/Scripts/Models/ContextPayload/Combat/Damage/DamageTakenData.cpp
    src/Scripts/Models/EventContext/EventContext.cpp
    src/Scripts/Models/EventContext/EventDepthTracker.cpp
    src/Scripts/Models/Expression.cpp
    src/Scripts/Models/EventQueue/EventQueue.cpp
    src/Scripts/States/ExpressionStore.cpp
    src/Scripts/Handlers/EventQueueHandler.cpp
    src/Scripts/Handlers/EventSorter.cpp
    src/Scripts/States/EventStore.cpp
    src/Scripts/Models/Events/CombatEvents/EventCombatBase.cpp
    src/Scripts/Models/Events/CombatEvents/Effects/ApplyStatusEffect.cpp
    src/Scripts/Models/ContextPayload/Utility/CancellationPayload.cpp
    src/Scripts/Models/ContextPayload/Utility/BanPayload.cpp
    src/Scripts/Helpers/Context/ContextBuilder.cpp
    src/Scripts/Models/EventContext/Combat/EventCombatContext.cpp
    src/Scripts/Handlers/EventFiring.cpp
    src/Scripts/Handlers/Targeting/TargetResolver.cpp
    src/Tests/ExpressionTests.cpp
    src/Tests/EventOpQueueTests.cpp
    src/Tests/EventStoreTests.cpp
    src/Tests/ContextPayloadTests.cpp
    src/Tests/EventQueueHandlerTests.cpp
    src/Tests/ContextDerivationTests.cpp
    src/Tests/ContextPayloadTests.cpp
    src/Tests/EventFiringTests.cpp
    src/Tests/EventSorterTests.cpp
    src/Tests/GatheringTests.cpp
    src/Tests/PropagationTests.cpp
    src/Tests/TargetResolverTests.cpp)

target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)
target_include_directories(tests PRIVATE src)
target_include_directories(data_driven_event_based_system PRIVATE src)