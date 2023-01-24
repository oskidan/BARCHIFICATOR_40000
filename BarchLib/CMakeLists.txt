enable_testing()

#***********************************************************************************************************************
# Catch2

include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY "https://github.com/catchorg/Catch2"
    GIT_TAG 	   "v3.2.1"
)
FetchContent_MakeAvailable(Catch2)

#***********************************************************************************************************************
# BrachLib

add_library(BarchLib STATIC)
target_sources(BarchLib 
    PUBLIC
        barchlib.hpp
    PRIVATE
        barchlib.cpp
)
target_include_directories(BarchLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(BarchLib PRIVATE BARCHLIB_LIBRARY)

#***********************************************************************************************************************
# BrachLibTests

add_executable(BarchLibTests)
target_sources(BarchLibTests  PRIVATE barchlib_test.cpp)
target_link_libraries(BarchLibTests 
    PRIVATE 
        BarchLib
        Catch2::Catch2WithMain
)
add_test(NAME BarchLibTests COMMAND BarchLibTests)

