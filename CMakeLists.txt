cmake_minimum_required(VERSION 3.7.2)

set(CMAKE_USE_RELATIVE_PATHS TRUE)
project("klib")
set("PROJECT_DESCRIPTION" "A standalone and lightweight C library that strives for efficiency and a small memory footprint, some of which components are among the most efficient
implementations of similar algorithms or data structures in all programming languages, in terms of both speed and memory use.")
set("PROJECT_HOMEPAGE_URL" "https://github.com/attractivechaos/klib")

set(CPACK_PACKAGE_VENDOR "klib developers")
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

set(OUR_CMAKE_MODULES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(OUR_CMAKE_3PARTY_MODULES_DIR "${OUR_CMAKE_MODULES_DIR}/thirdParty")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${OUR_CMAKE_MODULES_DIR}" "${OUR_CMAKE_3PARTY_MODULES_DIR}" "${OUR_CMAKE_3PARTY_MODULES_DIR}/Boilerplate" "${OUR_CMAKE_3PARTY_MODULES_DIR}/DetectDependentHeaders" "${OUR_CMAKE_3PARTY_MODULES_DIR}/DoxygenUtils")


include(Boilerplate)
include(DetectDependentHeaders)
include(DoxygenUtils)

set(CPACK_DEBIAN_ENABLE_COMPONENT_DEPENDS ON)
set(CPACK_RPM_ENABLE_COMPONENT_DEPENDS ON)

set(C_dir "${CMAKE_CURRENT_SOURCE_DIR}/C")
set(CPP_dir "${CMAKE_CURRENT_SOURCE_DIR}/cpp")
set(tests_dir "${CMAKE_CURRENT_SOURCE_DIR}/test")

add_subdirectory("${C_dir}")
add_subdirectory("${CPP_dir}")

option(WITH_EXAMPLES "Build examples" OFF)
if(WITH_EXAMPLES)
	#file(GLOB EXAMPLES "${tests_dir}/*.c" "${tests_dir}/*.cc" "${tests_dir}/*.cpp")
	file(GLOB EXAMPLES "${tests_dir}/*.c" "${tests_dir}/*.cpp")
	foreach(example ${EXAMPLES})
		detect_dependencies_by_headers("k" "${example}" EXAMPLE_DEPS)
		get_filename_component(exampleName "${example}" NAME_WE)
		add_executable("${exampleName}" "${example}")
		target_link_libraries("${exampleName}" PRIVATE ${PROJECT_NAME} ${EXAMPLE_DEPS})
	endforeach()

	find_package(ZLIB REQUIRED)
	target_link_libraries(kseq_test PRIVATE ZLIB::ZLIB)
	target_link_libraries(kseq_bench PRIVATE ZLIB::ZLIB)
endif()

option(WITH_DOCS "Build docs" OFF)
if(WITH_DOCS)
	find_package(Doxygen REQUIRED dot)
	load_doxyfile("${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile")

	set(DOXYGEN_PROJECT_BRIEF "${CPACK_PACKAGE_DESCRIPTION}")
	set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "${CPACK_RESOURCE_FILE_README}")

	set("DOXYGEN_GENERATE_HTML" YES)
	set("DOXYGEN_GENERATE_MAN" YES)

	if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		message(STATUS "Compiler is CLang, enabling CLang-assisted parsing in Doxygen.")
		set(DOXYGEN_CLANG_ASSISTED_PARSING YES)
		list(JOIN C_SUBLIBS " -I" DOXYGEN_CLANG_OPTIONS)
		set(DOXYGEN_CLANG_OPTIONS "-I${C_dir} -I${CPP_dir} -I${Include_dir} -I${DOXYGEN_CLANG_OPTIONS}")
	endif()

	set(DOXYGEN_EXAMPLE_PATH "${tests_dir}")
	set(DOXYGEN_STRIP_FROM_PATH "${Include_dir}")
	set(DOXYGEN_STRIP_FROM_INC_PATH "${Include_dir}")


	file(GLOB SOURCES_TO_DOCUMENT "${C_dir}/*.[hc]" "${CPP_dir}/*.[ch]xx" "${CPP_dir}/*.[ch]pp")
	doxygen_add_docs(docs
		"${SOURCES_TO_DOCUMENT}"
		ALL
		USE_STAMP_FILE
	)
endif()

#pass_through_cpack_vars()

include(CPack)
