set (SOURCES
	fast_cgi_data.cpp
	request.cpp
	router.cpp
	server.cpp
	symbol.cpp
	symbol_server.cpp
	symbols.cpp
)

set (PRIVATE_HEADERS
	fast_cgi_data.h
	symbol_server.h
)

set (HEADERS
	i_cgi_data.h
	i_router.h
	fcgiserver.h
	fcgiserver_defs.h
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
)

set (TEST_SOURCES
	test_mock_cgi_data.h
	test_mock_cgi_data.cpp
	test_request.cpp
	test_symbol.cpp
)

find_path(FCGI_INCLUDE_DIR NAMES fcgiapp.h PATH_SUFFIXES fcgi fastcgi REQUIRED)
find_library(FCGI_LIBRARY NAMES libfcgi fcgi REQUIRED)
find_package(Threads REQUIRED)

add_library(fcgiserver SHARED ${SOURCES} ${PRIVATE_HEADERS} ${HEADERS} ${OTHER})
target_include_directories(fcgiserver SYSTEM PRIVATE ${FCGI_INCLUDE_DIR})
target_include_directories(fcgiserver INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/fcgiserver>
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
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
	${CMAKE_SOURCE_DIR}/cmake/fcgiserverConfig.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/fcgiserverConfig.cmake
	INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake
)
install(
	FILES ${CMAKE_CURRENT_BINARY_DIR}/fcgiserverConfig.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake
)

catchtest(fcgiserver ${TEST_SOURCES})
