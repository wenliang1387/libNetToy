set(base_SRCS
    Buffer.cpp
    EventLoopThreadPool.cpp
    EventLoop.cpp
    Logger.cpp
    EventLoopThread.cpp
    TimerQueue.cpp   
)
add_library(toy_base ${base_SRCS})

install(TARGETS toy_base DESTINATION lib)
