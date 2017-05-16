set(net_SRCS
    Acceptor.cpp
    TcpClient.cpp
    TcpServer.cpp
    Connector.cpp
    TcpConnection.cpp
)
add_library(toy_net ${net_SRCS})
install(TARGETS toy_net DESTINATION lib)
