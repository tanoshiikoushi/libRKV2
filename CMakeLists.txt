cmake_minimum_required(VERSION 3.12)

include(CMakePackageConfigHelpers)

SET(lib_version 1.0)

project(libRKV2 VERSION ${lib_version} 
				DESCRIPTION "A library for manipulating RKV2 files"
				LANGUAGES CXX)
				
add_subdirectory(src)

configure_package_config_file(
	libRKV2Config.cmake.in
	libRKV2Config.cmake
	INSTALL_DESTINATION lib/cmake/libRKV2)

write_basic_package_version_file(
	libRKV2ConfigVersion.cmake
	VERSION ${lib_version} 
	COMPATIBILITY AnyNewerVersion)
	
install(TARGETS RKV2 
		ARCHIVE DESTINATION lib 
		INCLUDES DESTINATION include
		FILE libRKV2Config.cmake
		NAMESPACE libRKV2::
		DESTINATION lib/cmake/libRKV2)
	
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libRKV2Config.cmake"
			  "${CMAKE_CURRENT_BINARY_DIR}/libRKV2ConfigVersion.cmake"
		DESTINATION lib/cmake/libRKV2)