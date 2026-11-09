cmake_minimum_required(VERSION 3.31)

project(neulang)

add_library(neu_impl)
target_include_directories(neu_impl PUBLIC "${NEU_INCLUDES}")
target_sources(neu_impl PRIVATE
    "${NEU_SRCS}/strs.c"
    "${NEU_SRCS}/dynbuf.c"
    "${NEU_SRCS}/token.c"
    "${NEU_SRCS}/lexer.c"
)

add_executable(neu "${NEU_SRCS}/main.c")
target_include_directories(neu PRIVATE "${NEU_INCLUDES}")
target_link_libraries(neu PRIVATE neu_impl m)
