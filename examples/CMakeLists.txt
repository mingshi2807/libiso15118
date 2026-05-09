add_executable(example_ac_der_secc_provider ac_der_secc_provider.cpp)

target_link_libraries(example_ac_der_secc_provider
    PRIVATE
        iso15118::iso15118
)
