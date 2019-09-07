#include <arpa/inet.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdio>
#include <vector>

#include "bipolar/io/io_uring.hpp"

#define MAX_CONN 1000
#define MAX_MSG 1000
#define PORT 9999
#define HOST "127.0.0.1"

using namespace bipolar;

const std::uint64_t LISTEN = 0, ECHO_RECV = 1, ECHO_SEND = 2, ECHO = 3;

#define GET_TYPE(x) ((x) >> 48)
#define GET_VALUE(x) ((x)&0xffffffffffffull)
#define TYPE_VALUE(t, v) (((t) << 48) | (v))

struct Connection {
    int fd;
    struct msghdr msg[2];
    struct iovec iov[2];
    char buf[MAX_MSG];
};

#define RX 0
#define TX 1

Connection conns[MAX_CONN];

int main() {
    for (std::size_t i = 0; i < MAX_CONN; ++i) {
        std::memset(&conns[i], 0, sizeof(conns[i]));
        // RX
        conns[i].iov[RX].iov_base = conns[i].buf;
        conns[i].iov[RX].iov_len = MAX_MSG;
        conns[i].msg[RX].msg_iov = &conns[i].iov[RX];
        conns[i].msg[RX].msg_iovlen = 1;

        // TX
        conns[i].iov[TX].iov_base = conns[i].buf;
        conns[i].iov[TX].iov_len = 0;
        conns[i].msg[TX].msg_iov = &conns[i].iov[TX];
        conns[i].msg[TX].msg_iovlen = 1;
    }

    struct io_uring_params p {};
    IOUring ring(200, &p);

    struct sockaddr_in saddr;
    std::memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr.sin_port = htons(PORT);

    int sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (sock < 0) {
        std::perror("socket");
        exit(-1);
    }

    const int enable_socket_reuseaddr = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable_socket_reuseaddr,
               sizeof(enable_socket_reuseaddr));

    if (bind(sock, (struct sockaddr*)&saddr, sizeof(saddr)) < 0) {
        std::perror("bind");
        exit(-1);
    }

    if (listen(sock, 100) < 0) {
        std::perror("listen");
        exit(-1);
    }

    ring.get_submission_entry().and_then(
        [sock, &ring](IOUringSQE& sqe) -> Result<Void, Void> {
            sqe.poll_add(sock, POLLIN);
            sqe.user_data = TYPE_VALUE(LISTEN, sock);
            std::puts("polling listen fd");
            return ring.submit(1);
        });

    while (auto result = ring.get_completion_entry()) {
        IOUringCQE& cqe = result.value();

        auto type = GET_TYPE(cqe.user_data);
        auto value = GET_VALUE(cqe.user_data);
        switch (type) {
        case LISTEN:
            if ((cqe.res & POLLIN) == POLLIN) {
                const int sock = value;
                ring.get_submission_entry().and_then(
                    [sock, &ring](IOUringSQE& sqe) -> Result<Void, Void> {
                        sqe.poll_add(sock, POLLIN);
                        sqe.user_data = TYPE_VALUE(LISTEN, sock);
                        std::puts("polling listen fd again");
                        return Ok(Void{});
                    });

                struct sockaddr_in addr;
                socklen_t len;

                int fd;
                while ((fd = accept4(sock, (struct sockaddr*)&addr, &len,
                                     SOCK_NONBLOCK)) != -1) {
                    if (fd >= MAX_CONN) {
                        close(fd);
                        break;
                    }

                    std::puts("LISTEN: poll client_fd submited");
                    ring.get_submission_entry().and_then(
                        [fd](IOUringSQE& sqe) -> Result<Void, Void> {
                            sqe.poll_add(fd, POLLIN);
                            sqe.user_data = TYPE_VALUE(ECHO, fd);
                            return Ok(Void{});
                        });
                }
            }
            break;

        case ECHO:
            if ((cqe.res & POLLIN) == POLLIN) {
                // const int fd = value;
                // int sz = read(fd, conns[fd].buf, MAX_MSG);
                // write(fd, conns[fd].buf, sz);
                //
                // ring.get_submission_entry().and_then(
                //     [fd](IOUringSQE& sqe) -> Result<Void, Void> {
                //         sqe.poll_add(fd, POLLIN);
                //         sqe.user_data = TYPE_VALUE(ECHO, fd);
                //         return Ok(Void{});
                //     });

                ring.get_submission_entry().and_then(
                    [fd = value](IOUringSQE& sqe) -> Result<Void, Void> {
                        sqe.recvmsg(fd, &conns[fd].msg[RX], 1);
                        sqe.user_data = TYPE_VALUE(ECHO_RECV, fd);
                        return Ok(Void{});
                    });
            }
            break;

        case ECHO_RECV:
            if (cqe.res == -1 || cqe.res == 0) {
                close(value);
            } else {
                // std::puts("ECHO_RECV: sendmsg submited");
                conns[value].iov[TX].iov_len = cqe.res;
                ring.get_submission_entry().and_then(
                    [fd = value](IOUringSQE& sqe) -> Result<Void, Void> {
                        sqe.sendmsg(fd, &conns[fd].msg[TX], 1);
                        sqe.user_data = TYPE_VALUE(ECHO_SEND, fd);
                        return Ok(Void{});
                    });
            }
            break;

        case ECHO_SEND:
            // std::puts("ECHO_SEND: poll client_fd submited");
            ring.get_submission_entry().and_then(
                [fd = value](IOUringSQE& sqe) -> Result<Void, Void> {
                    sqe.poll_add(fd, POLLIN);
                    sqe.user_data = TYPE_VALUE(ECHO, fd);
                    return Ok(Void{});
                });
        }

        ring.submit();
        ring.seen(1);
    }

    return 0;
}
