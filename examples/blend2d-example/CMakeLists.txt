cmake_minimum_required(VERSION 3.2)

project(blend2d-example)

set(MYSVG_DIR "../../include")

set(CMAKE_CXX_STANDARD 14)

add_executable(blend2d-example
	"Source.cpp"
	"../../bindings/Renderer/MySVG_Blend2d_Impl.cpp"
)

add_library(MySVG INTERFACE)
target_include_directories(MySVG INTERFACE ${MYSVG_DIR})

list(APPEND EXTRA_LIBS MySVG)
list(APPEND EXTRA_LIBS "blend2d")

target_link_libraries(blend2d-example PUBLIC ${EXTRA_LIBS})
