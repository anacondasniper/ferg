/*
 * Ferg wire protocol, version 1
 *
 * One file per connection. All multi-byte numbers are written big-endian
 *
 * offset  		   size      field        notes
 * ______  		   ____      _____        ___________________________________________
 * 0	   		   4         magic		  FERG
 * 4	   		   1         version	  the version of ferg being used, 1
 * 5	   		   1	     name_len	  1 .. max 255 ; 0 is invalid
 * 6	   		   name_len  name 		  name_len bytes, no terminator
 * 6 + name_len	   8 	     file_size	  bytes of file data
 * 14 + name_len   file_size data		  file_size bytes, actual data we want to transfer
 * Receiver must reject (and close) if:
 *   - magic or version doesn't match
 *   - name_len is 0 or above the max
 *   - name contains '/', '\\', or ".."   (or is exactly "." or "..")
 *   - the connection closes before file_size bytes arrive
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <cerrno>
#include <cstdio>
#include <sys/socket.h>

inline bool send_all(int fd, const void *buf, size_t n)
{
    size_t done = 0;
    while (done < n)
    {
        ssize_t s = send(fd, reinterpret_cast<const uint8_t *>(buf) + done, n - done, MSG_NOSIGNAL);
        if (s == -1)
        {
            if (errno == EINTR)
                continue;
            perror("send");
            return false;
        }
        done += s;
    }
    return true;
}

inline bool recv_all(int fd, void *buf, size_t n)
{
    size_t done = 0;
    while (done < n)
    {
        ssize_t r = recv(fd, reinterpret_cast<uint8_t *>(buf) + done, n - done, MSG_NOSIGNAL);
        if (r == -1)
        {
            if (errno == EINTR)
                continue;
            perror("recv");
            return false;
        }
        if (r == 0)
        {
            fprintf(stderr, "connection closed early\n");
            return false;
        }
        done += r;
    }
    return true;
}
