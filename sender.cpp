#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdio>

int main(int argc, char **argv)
{
    if (!argc == 2)
    {
        fprintf(stderr, "Usage: sender [filename]");
        return 1;
    }
    struct sockaddr_in addr =
        {
            AF_INET,
            htons(7000)};
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    int send_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (send_fd == -1)
    {
        perror("socket");
        return 1;
    }
    int connection = connect(send_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
    if (connection == -1)
    {
        perror("connection refused");
        return 1;
    }
}
