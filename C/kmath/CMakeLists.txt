buildAndPackageLib("kmath"
	COMPONENT "kmath"
	DESCRIPTION "${PROJECT_DESCRIPTION}\n\nA numerical routines including MT19937-64 pseudorandom generator, basic nonlinear programming and a few special math functions."
	CMAKE_EXPORT_NAMESPACE "${PROJECT_NAME}"
	PUBLIC_INCLUDES "${CMAKE_CURRENT_LIST_DIR}"
	INSTALL_INCLUDE_DIR "${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}"
)

if(MSVC)
else()
	target_link_libraries(kmath PUBLIC m)
endif()
