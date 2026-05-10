include(Catch)

add_executable(test_timeouts timeouts.cpp)
add_executable(test_ac_der_control ac_der_control.cpp)

target_link_libraries(test_timeouts
    PRIVATE
        iso15118
        Catch2::Catch2WithMain
)

target_link_libraries(test_ac_der_control
    PRIVATE
        iso15118
        Catch2::Catch2WithMain
)

catch_discover_tests(test_timeouts)
catch_discover_tests(test_ac_der_control)
