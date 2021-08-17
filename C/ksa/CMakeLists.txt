set(DESCR "Constructs suffix arrays for strings with multiple sentinels, based on a revised SAIS algorithm.")

buildAndPackageLib("ksa32"
	COMPONENT "ksa32"
	DESCRIPTION "${PROJECT_DESCRIPTION}\n\n${DESCR}\n\nUses 32-bit integers."
	CMAKE_EXPORT_NAMESPACE "${PROJECT_NAME}"
	PUBLIC_INCLUDES "${CMAKE_CURRENT_LIST_DIR}"
	INSTALL_INCLUDE_DIR "${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}"
)

buildAndPackageLib("ksa64"
	COMPONENT "ksa64"
	DESCRIPTION "${PROJECT_DESCRIPTION}\n\n${DESCR}\n\nUses 64-bit integers."
	CMAKE_EXPORT_NAMESPACE "${PROJECT_NAME}"
	PUBLIC_INCLUDES "${CMAKE_CURRENT_LIST_DIR}"
	INSTALL_INCLUDE_DIR "${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}"
)

target_compile_definitions(ksa64 PRIVATE "-D_KSA64")
