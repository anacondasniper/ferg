# ferg

**F**ile **E**xchange **R**elay **G**adget: a small TCP file transfer tool written in C++.

Run `receiver` on one machine and `sender` on another, and a file goes from one to the other. Built to learn sockets and network protocol design.

## Status

- [x] Receiver: listens on port 7000, accepts a connection, reads the stream
- [ ] Receiver writes to a file
- [ ] Sender
- [ ] Protocol header (filename and size)
- [ ] Error handling

## Build

    make receiver
    make sender

Needs g++ with C++20. Builds with warnings and ASan/UBSan enabled.

## Usage

    ./receiver
    nc -N localhost 7000 < somefile
