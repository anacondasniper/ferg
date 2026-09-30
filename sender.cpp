#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdio>

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "usage: sender <file>\n");
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(7000);
    if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) != 1)
    {
        fprintf(stderr, "bad address\n");
        return 1;
    }

    int send_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (send_fd == -1)
    {
        perror("socket");
        return 1;
    }

    if (connect(send_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == -1)
    {
        perror("connect");
        close(send_fd);
        return 1;
    }

    FILE *send_file = fopen(argv[1], "rb");
    if (!send_file)
    {
        perror("fopen");
        close(send_fd);
        return 1;
    }

    char send_buf[4096];
    size_t byte_counter = 0;

    while (true)
    {
        size_t read_bytes = fread(send_buf, 1, sizeof(send_buf), send_file);
        if (read_bytes == 0)
            break;

        size_t sent = 0;
        while (sent < read_bytes)
        {
            ssize_t r = send(send_fd, send_buf + sent, read_bytes - sent, MSG_NOSIGNAL);
            if (r < 0)
            {
                perror("send");
                fclose(send_file);
                close(send_fd);
                return 1;
            }
            sent += static_cast<size_t>(r);
        }
        byte_counter += sent;
    }

    if (ferror(send_file))
    {
        perror("fread");
        fclose(send_file);
        close(send_fd);
        return 1;
    }

    printf("sent %zu bytes\n", byte_counter);
    fclose(send_file);
    close(send_fd);
    return 0;
}
