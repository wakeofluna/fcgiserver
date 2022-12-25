set (SOURCES
	console_log_callback.cpp
	fast_cgi_data.cpp
	i_log_callback.cpp
	line_formatter.cpp
	logger.cpp
	request.cpp
	router.cpp
	server.cpp
	symbol.cpp
	symbol_server.cpp
	symbols.cpp
)

set (PRIVATE_HEADERS
	console_log_callback.h
	fast_cgi_data.h
	symbol_server.h
)

set (HEADERS
	i_cgi_data.h
	i_log_callback.h
	i_router.h
	fcgiserver.h
	fcgiserver_defs.h
	line_formatter.h
	logger.h
	request.h
	request_method.h
	router.h
	server.h
	symbol.h
	symbols.h
)

set (OTHER
	${CMAKE_SOURCE_DIR}/cmake/fcgiserverConfig.cmake.in
	${CMAKE_SOURCE_DIR}/README.md
	${CMAKE_SOURCE_DIR}/LICENSE
)

set (TEST_SOURCES
	test_mock_cgi_data.h
	test_mock_cgi_data.cpp
	test_request.cpp
	test_router.cpp
	test_symbol.cpp
)

find_path(FCGI_INCLUDE_DIR NAMES fcgiapp.h PATH_SUFFIXES fcgi fastcgi REQUIRED)
find_library(FCGI_LIBRARY NAMES libfcgi fcgi REQUIRED)
find_package(Threads REQUIRED)

add_library(fcgiserver SHARED ${SOURCES} ${PRIVATE_HEADERS} ${HEADERS} ${OTHER})
target_include_directories(fcgiserver SYSTEM PRIVATE ${FCGI_INCLUDE_DIR})
target_include_directories(fcgiserver INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(fcgiserver PRIVATE ${FCGI_LIBRARY} Threads::Threads)

include(GNUInstallDirs)
set_target_properties(fcgiserver
	PROPERTIES
	PUBLIC_HEADER "${HEADERS}"
)
install (TARGETS fcgiserver
	EXPORT fcgiserverTargets
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/fcgiserver
)
install(
	EXPORT fcgiserverTargets
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/fcgiserver
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
	${CMAKE_SOURCE_DIR}/cmake/fcgiserverConfig.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/fcgiserverConfig.cmake
	INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/fcgiserver
)
install(
	FILES ${CMAKE_CURRENT_BINARY_DIR}/fcgiserverConfig.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/fcgiserver
)

catchtest(fcgiserver ${TEST_SOURCES})
