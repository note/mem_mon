set(EXTRA_LIBS ${EXTRA_LIBS} mem_mon)

add_executable (libtest libtest.cpp)
add_executable (basic basic.cpp)
add_executable (mthreaded mthreaded.cpp)
add_executable (mthreaded2 mthreaded2.cpp)
add_executable (signals signals.cpp)

target_link_libraries(libtest ${EXTRA_LIBS})
target_link_libraries(basic rt)
target_link_libraries(mthreaded pthread)
target_link_libraries(mthreaded2 pthread)

