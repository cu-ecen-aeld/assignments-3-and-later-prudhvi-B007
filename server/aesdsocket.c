#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <fcntl.h>
#include <signal.h>

#define PORT 9000
#define FILE_PATH "/var/tmp/aesdsocketdata"
#define BUF_SIZE 1024


int server_fd;  // make global so handler can close it

void signal_handler(int signo)
{
    if (signo == SIGINT || signo == SIGTERM) {
        syslog(LOG_INFO, "Caught signal, exiting");

        if (server_fd >= 0) {
            close(server_fd);
        }

        // delete the file
        remove(FILE_PATH);

        closelog();
        exit(EXIT_SUCCESS);
    }
}

void daemonize()
{
    pid_t pid = fork();

    if (pid < 0) {
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        // Parent exits
        exit(EXIT_SUCCESS);
    }

    // Child continues
    if (setsid() < 0) {
        exit(EXIT_FAILURE);
    }

    // Redirect standard file descriptors to /dev/null
    freopen("/dev/null", "r", stdin);
    freopen("/dev/null", "w", stdout);
    freopen("/dev/null", "w", stderr);
}


int main(int argc, char *argv[])
{

    int daemon_mode = 0;

    if (argc == 2 && strcmp(argv[1], "-d") == 0) {
        daemon_mode = 1;
    }

    if (daemon_mode) {
        daemonize();
    }

    int server_fd, client_fd, data_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUF_SIZE];
    ssize_t bytes_read;

    openlog("aesdsocket", LOG_PID, LOG_USER);
    
    // Install signal handlers
    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        syslog(LOG_ERR, "Error setting SIGINT handler");
        exit(EXIT_FAILURE);
    }
    if (signal(SIGTERM, signal_handler) == SIG_ERR) {
        syslog(LOG_ERR, "Error setting SIGTERM handler");
        exit(EXIT_FAILURE);
    }


    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        syslog(LOG_ERR, "socket failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        syslog(LOG_ERR, "bind failed: %s", strerror(errno));
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) == -1) {
        syslog(LOG_ERR, "listen failed: %s", strerror(errno));
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd == -1) {
            syslog(LOG_ERR, "accept failed: %s", strerror(errno));
            continue;
        }

        syslog(LOG_INFO, "Accepted connection from %s", inet_ntoa(client_addr.sin_addr));

        // Open file for append
        data_fd = open(FILE_PATH, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (data_fd == -1) {
            syslog(LOG_ERR, "open file failed: %s", strerror(errno));
            close(client_fd);
            continue;
        }

        // Read until newline
        while ((bytes_read = recv(client_fd, buffer, BUF_SIZE, 0)) > 0) {
            write(data_fd, buffer, bytes_read);
            if (memchr(buffer, '\n', bytes_read)) {
                break;
            }
        }
        close(data_fd);

        // Send back full file contents
        data_fd = open(FILE_PATH, O_RDONLY);
        if (data_fd != -1) {
            while ((bytes_read = read(data_fd, buffer, BUF_SIZE)) > 0) {
                send(client_fd, buffer, bytes_read, 0);
            }
            close(data_fd);
        }

        close(client_fd);
        syslog(LOG_INFO, "Closed connection from %s", inet_ntoa(client_addr.sin_addr));
    }

    close(server_fd);
    closelog();
    return 0;
}
