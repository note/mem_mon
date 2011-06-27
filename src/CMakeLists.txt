add_library(mem_mon process.cpp system.cpp common.cpp)

add_executable (memtrace memtrace.cpp)
target_link_libraries(memtrace mem_mon)

install (TARGETS mem_mon DESTINATION lib)
install (FILES ../include/mem_mon.h ../include/process.h ../include/system.h
../include/common.h DESTINATION include/mem_mon)
install (TARGETS memtrace DESTINATION bin)
