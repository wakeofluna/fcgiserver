set (SOURCES
    fast_cgi_data.cpp
    request.cpp
    server.cpp
)

set (HEADERS
    i_cgi_data.h
    fast_cgi_data.h
    logger.h
    request.h
    request_method.h
    server.h
)

set (TEST_SOURCES
    test_mock_cgi_data.h
    test_mock_cgi_data.cpp
    test_request.cpp
)

find_path(FCGI_INCLUDE_DIR NAMES fcgiapp.h PATH_SUFFIXES fcgi fastcgi REQUIRED)
find_library(FCGI_LIBRARY NAMES libfcgi fcgi REQUIRED)

add_library(fcgiserver-lib STATIC ${SOURCES} ${HEADERS})
target_include_directories(fcgiserver-lib SYSTEM PUBLIC ${FCGI_INCLUDE_DIR})
target_include_directories(fcgiserver-lib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(fcgiserver-lib PUBLIC ${FCGI_LIBRARY})

catchtest(fcgiserver-lib ${TEST_SOURCES})
