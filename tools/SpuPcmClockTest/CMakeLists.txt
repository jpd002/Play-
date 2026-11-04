add_executable(SpuPcmClockTest Main.cpp)
target_link_libraries(SpuPcmClockTest PRIVATE PlayCore)
add_test(NAME SpuPcmClockTest COMMAND SpuPcmClockTest)
