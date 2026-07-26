add_executable(dbtest DatabaseTest.cpp)

target_link_libraries(dbtest PRIVATE
                      grooveoff_core
                      Qt6::Test)

add_test(NAME database COMMAND dbtest)
