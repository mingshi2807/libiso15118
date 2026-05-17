add_executable(example_ac_der_secc_provider ac_der_secc_provider.cpp)
add_executable(example_ac_der_secc_application_adapter ac_der_secc_application_adapter.cpp)

target_link_libraries(example_ac_der_secc_provider
    PRIVATE
        iso15118::iso15118
)

target_link_libraries(example_ac_der_secc_application_adapter
    PRIVATE
        iso15118::iso15118
)
