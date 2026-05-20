add_library(iso15118_c STATIC iso15118_c.cpp)

target_include_directories(iso15118_c
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(iso15118_c
    PUBLIC
        iso15118
        OpenSSL::SSL
        OpenSSL::Crypto
)

target_compile_features(iso15118_c PUBLIC cxx_std_17)

set_target_properties(iso15118_c PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
