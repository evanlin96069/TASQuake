list(APPEND CLIENT_TEST_SOURCES
  "client.cpp"
)

list(APPEND STRESS_CLIENT_TEST_SOURCES
  "stress_client.cpp"
)

list(APPEND SERVER_TEST_SOURCES
  "server.cpp"
)

add_executable(client_test ${CLIENT_TEST_SOURCES})
target_link_libraries(client_test PRIVATE libtasquake)
add_executable(server_test ${SERVER_TEST_SOURCES})
target_link_libraries(server_test PRIVATE libtasquake)
add_executable(client_stress_test ${STRESS_CLIENT_TEST_SOURCES})
target_link_libraries(client_stress_test PRIVATE libtasquake)