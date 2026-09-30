#include <cstring>
#include <cstdint>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdio>
#include "protocol.h"

int main()
{
    constexpr uint8_t MAGIC[4] = {'F', 'E', 'R', 'G'};
    constexpr uint8_t VERSION = 1;

    int reuse_addr = 1;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(7000);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int recv_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (recv_fd == -1)
    {
        perror("socket");
        return 1;
    }
    if (setsockopt(recv_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr)) == -1)
    {
        perror("setsockopt");
        close(recv_fd);
        return 1;
    }
    if (bind(recv_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == -1)
    {
        perror("bind");
        close(recv_fd);
        return 1;
    }
    if (listen(recv_fd, 10) == -1)
    {
        perror("listen");
        close(recv_fd);
        return 1;
    }
    printf("listening on port 7000\n");

    int client_fd = accept(recv_fd, nullptr, nullptr);
    if (client_fd == -1)
    {
        perror("accept");
        close(recv_fd);
        return 1;
    }

    uint8_t magic[4];
    uint8_t version;
    uint8_t name_len;
    char name[256];
    uint8_t size_bytes[8];
    uint64_t file_size = 0;
    char recv_buf[4096];
    size_t byte_counter = 0;

    // header: every read is one field, every field is checked as soon as it arrives
    if (!recv_all(client_fd, magic, sizeof(magic)))
    {
        close(client_fd);
        close(recv_fd);
        return 1;
    }
    if (memcmp(magic, MAGIC, sizeof(MAGIC)) != 0)
    {
        fprintf(stderr, "rejected: bad magic\n");
        close(client_fd);
        close(recv_fd);
        return 1;
    }
    if (!recv_all(client_fd, &version, 1))
    {
        close(client_fd);
        close(recv_fd);
        return 1;
    }
    if (version != VERSION)
    {
        fprintf(stderr, "rejected: bad version\n");
        close(client_fd);
        close(recv_fd);
        return 1;
    }
    if (!recv_all(client_fd, &name_len, 1))
    {
        close(client_fd);
        close(recv_fd);
        return 1;
    }
    if (name_len == 0)
    {
        fprintf(stderr, "rejected: empty name\n");
        close(client_fd);
        close(recv_fd);
        return 1;
    }
    if (!recv_all(client_fd, name, name_len))
    {
        close(client_fd);
        close(recv_fd);
        return 1;
    }
    name[name_len] = '\0'; // terminate BEFORE any string function touches it
    if (strlen(name) != name_len)
    {
        fprintf(stderr, "rejected: name contains a null byte\n");
        close(client_fd);
        close(recv_fd);
        return 1;
    }
    if (strchr(name, '/') != nullptr || strchr(name, '\\') != nullptr ||
        strstr(name, "..") != nullptr || strcmp(name, ".") == 0)
    {
        fprintf(stderr, "rejected: illegal name\n");
        close(client_fd);
        close(recv_fd);
        return 1;
    }
    if (!recv_all(client_fd, size_bytes, sizeof(size_bytes)))
    {
        close(client_fd);
        close(recv_fd);
        return 1;
    }
    for (int i = 0; i < 8; i++)
    {
        file_size = (file_size << 8) | size_bytes[i];
    }

    // only now, with a validated name, create the file
    FILE *recv_file = fopen(name, "wx");
    if (!recv_file)
    {
        perror("fopen");
        close(client_fd);
        close(recv_fd);
        return 1;
    }

    uint64_t remaining = file_size;
    while (remaining > 0)
    {
        size_t chunk = remaining < sizeof(recv_buf) ? static_cast<size_t>(remaining) : sizeof(recv_buf);
        if (!recv_all(client_fd, recv_buf, chunk))
        {
            fprintf(stderr, "transfer failed: connection lost before the file was complete\n");
            fclose(recv_file);
            remove(name);
            close(client_fd);
            close(recv_fd);
            return 1;
        }
        if (fwrite(recv_buf, 1, chunk, recv_file) != chunk)
        {
            fprintf(stderr, "transfer failed: could not write to disk\n");
            fclose(recv_file);
            remove(name);
            close(client_fd);
            close(recv_fd);
            return 1;
        }
        remaining -= chunk;
        byte_counter += chunk;
    }

    printf("received %zu bytes into %s\n", byte_counter, name);
    fclose(recv_file);
    close(client_fd);
    close(recv_fd);
    return 0;
}
