cmake_minimum_required(VERSION 3.8)

file(GLOB_RECURSE APP_SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(GameBoyApp ${APP_SRC})

target_link_libraries(GameBoyApp PUBLIC GameBoy Imgui NativeFileDialog ${SDL2_LIBRARIES})
if(WIN32)
    add_custom_command(
        TARGET GameBoyApp POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:SDL2::SDL2>"
            "$<TARGET_FILE_DIR:GameBoyApp>"
        VERBATIM
    )
endif()