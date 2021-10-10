set (SOURCES
    main.cpp
)

set (HEADERS

)

find_package(Threads REQUIRED)

add_executable(fcgiserver ${SOURCES} ${HEADERS})
target_link_libraries(fcgiserver PRIVATE fcgiserver-lib Threads::Threads)

