# RWG WebSocket
该框架实现简易HTTP协议及WebSocket协议。实现方式由epoll实现I/O多路复用。
服务将开启n个线程（包括主线程），$n = _SC_NPROCESSORS_CONF + 1$。
主线程将用于接收客户端请求，并分配到子线程中。子线程将处理传入数据、中断等事件。

