cmake_minimum_required(VERSION 3.31)

project(neulang)

add_library(neu_impl)
target_include_directories(neu_impl PUBLIC "${NEU_INCLUDES}")
target_sources(neu_impl PRIVATE
    "${NEU_SRCS}/strs.c"
    "${NEU_SRCS}/dynbuf.c"
    "${NEU_SRCS}/token.c"
    "${NEU_SRCS}/lexer.c"
    "${NEU_SRCS}/ast.c"
    "${NEU_SRCS}/parser.c"
    "${NEU_SRCS}/pool.c"
    "${NEU_SRCS}/bytecode.c"
    "${NEU_SRCS}/bcc.c"
    "${NEU_SRCS}/var_scope.c"
)

add_executable(neu "${NEU_SRCS}/main.c")
target_include_directories(neu PRIVATE "${NEU_INCLUDES}")
target_link_libraries(neu PRIVATE neu_impl m)
