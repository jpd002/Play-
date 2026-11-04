add_executable(NamcoAcCdvdTest Main.cpp)
target_link_libraries(NamcoAcCdvdTest PRIVATE PlayCore)
add_test(NAME NamcoAcCdvdTest COMMAND NamcoAcCdvdTest)
