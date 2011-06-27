add_library(mem_mon process.cpp system.cpp)

add_executable (memtrace memtrace.cpp)

install (TARGETS mem_mon DESTINATION lib)
install (FILES ../include/mem_mon.h ../include/process.h ../include/system.h
../include/common.h DESTINATION include/mem_mon)
install (TARGETS memtrace DESTINATION bin)
