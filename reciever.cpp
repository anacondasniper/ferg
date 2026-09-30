#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdio>

int main()
{
    int reuse_addr = 1;
    struct sockaddr_in addr =
        {
            AF_INET,
            htons(7000),
            htonl(INADDR_ANY)};
    // 1. Make a socket - socket() - "give me an endpoint I can use for the network"
    int recv_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (recv_fd == -1)
    {
        perror("socket");
        return 1;
    }
    // 2. Allow quick restarts - setsockopt() - "let me reuse the port right after I quit"
    int recv_opt = setsockopt(recv_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(int));
    if (recv_opt == -1)
    {
        perror("sockopt");
        return 1;
    }
    // 3. Attach it to a port (7000) - bind() - "this program owns this port"
    int binding = bind(recv_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
    if (binding == -1)
    {
        perror("binding");
        return 1;
    }
    // 4. start listening on the port - listen() - "i'm ready for connections on this socket"
    int listening = listen(recv_fd, 10);
    if (listening == -1)
    {
        perror("listener");
        return 1;
    }
    printf("listening on port 7000\n");
    // 5. await connection - accept() - "wait for a connection, hand me the client's socket"
    int client_fd = accept(recv_fd, nullptr, nullptr);
    if (client_fd == -1)
    {
        perror("can't connect to client");
        return 1;
    }
    // 6. read what they send - recv() - gives bytes of message back
    char recv_buf[4096];
    size_t byte_counter = 0;
    while (true)
    {
        ssize_t recv_byte_t = recv(client_fd, recv_buf, sizeof(recv_buf), 0);
        if (recv_byte_t == 0)
            break;
        else if (recv_byte_t < 0)
        {
            perror("can't resolve message");
            break;
        }
        else if (recv_byte_t > 0)
        {
            fwrite(recv_buf, 1, recv_byte_t, stdout);
            byte_counter += recv_byte_t;
        }
    }
    printf("\nrecieved %zu bytes\n", byte_counter);
    // 8. close both sockets
    close(recv_fd);
    close(client_fd);
}
