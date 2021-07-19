file(GLOB CPP_SOURCES "*.cpp")
file(GLOB H_HEADERS "../include/*.h")

add_library(RKV2 STATIC ${CPP_SOURCES} ${H_HEADERS})
target_include_directories(RKV2 PUBLIC ../include)
target_compile_features(RKV2 PUBLIC cxx_std_17)
set_target_properties(RKV2 PROPERTIES CXX_EXTENSIONS OFF)
target_link_libraries(RKV2 KoushiCommon -static)

source_group (
	TREE "${PROJECT_SOURCE_DIR}/include"
	PREFIX "Header Files"
	FILES ${H_HEADERS})