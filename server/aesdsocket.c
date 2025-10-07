#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <time.h>

#define PORT 9000
#define DATAFILE "/var/tmp/aesdsocketdata"
#define BUF_SIZE 1024

static int server_fd = -1;
static volatile sig_atomic_t stop = 0;
pthread_mutex_t file_lock = PTHREAD_MUTEX_INITIALIZER;

/* Linked list to keep track of threads */
struct client_thread {
    pthread_t tid;
    int fd;
    SLIST_ENTRY(client_thread) entries;
};
SLIST_HEAD(thread_list, client_thread) head = SLIST_HEAD_INITIALIZER(head);

/* Signal handler */
void signal_handler(int sig) {
    stop = 1;
    if (server_fd != -1) {
        close(server_fd); // unblock accept
        server_fd = -1;
    }
}

/* Write whole buffer */
ssize_t write_all(int fd, const void *buf, size_t len) {
    const char *p = buf;
    while (len > 0) {
        ssize_t w = write(fd, p, len);
        if (w < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        p += w;
        len -= w;
    }
    return 0;
}

/* Append to file with lock */
void append_to_file(const void *buf, size_t len) {
    pthread_mutex_lock(&file_lock);
    int fd = open(DATAFILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd >= 0) {
        (void)write_all(fd, buf, len);
        close(fd);
    }
    pthread_mutex_unlock(&file_lock);
}

/* Send file contents back */
void send_file(int cfd) {
    pthread_mutex_lock(&file_lock);
    int fd = open(DATAFILE, O_RDONLY);
    if (fd >= 0) {
        char buf[BUF_SIZE];
        ssize_t n;
        while ((n = read(fd, buf, sizeof(buf))) > 0) {
            (void)send(cfd, buf, n, 0);
        }
        close(fd);
    }
    pthread_mutex_unlock(&file_lock);
}

/* Thread to handle each client */
void *client_handler(void *arg) {
    int cfd = *(int *)arg;
    free(arg);

    char buf[BUF_SIZE];
    ssize_t n;
    while ((n = recv(cfd, buf, sizeof(buf), 0)) > 0) {
        append_to_file(buf, n);
        if (memchr(buf, '\n', n)) {
            send_file(cfd);
        }
    }

    close(cfd);
    return NULL;
}

/* Thread to write timestamp every 10s */
void *timer_thread(void *arg) {
    (void)arg;
    while (!stop) {
        sleep(10);
        if (stop) break;

        time_t t = time(NULL);
        struct tm tm;
        localtime_r(&t, &tm);

        char line[128];
        int len = strftime(line, sizeof(line), "timestamp: %a, %d %b %Y %T %z\n", &tm);
        if (len > 0) {
            append_to_file(line, (size_t)len);
        }
    }
    return NULL;
}

/* Daemon mode */
void daemonize(void) {
    pid_t pid = fork();
    if (pid < 0) exit(1);
    if (pid > 0) exit(0);

    if (setsid() < 0) exit(1);

    pid = fork();
    if (pid < 0) exit(1);
    if (pid > 0) exit(0);

    umask(0);

    if (chdir("/") != 0) {
        perror("chdir");
        exit(1);
    }

    if (freopen("/dev/null", "r", stdin) == NULL) {
        perror("freopen stdin");
        exit(1);
    }
    if (freopen("/dev/null", "w", stdout) == NULL) {
        perror("freopen stdout");
        exit(1);
    }
    if (freopen("/dev/null", "w", stderr) == NULL) {
        perror("freopen stderr");
        exit(1);
    }
}


/* Main */
int main(int argc, char *argv[]) {
    int daemon = (argc == 2 && strcmp(argv[1], "-d") == 0);

    int fd = open(DATAFILE, O_CREAT | O_TRUNC | O_WRONLY, 0644);
    if (fd >= 0) close(fd);

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    if (daemon) daemonize();

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); exit(1); }

    int yes = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); exit(1);
    }
    if (listen(server_fd, 10) < 0) {
        perror("listen"); exit(1);
    }

    pthread_t timer_tid;
    pthread_create(&timer_tid, NULL, timer_thread, NULL);

    SLIST_INIT(&head);

    while (!stop) {
        int *cfd = malloc(sizeof(int));
        if (!cfd) continue;

        *cfd = accept(server_fd, NULL, NULL);
        if (*cfd < 0) {
            free(cfd);
            if (errno == EINTR && stop) break;
            continue;
        }

        struct client_thread *node = malloc(sizeof(*node));
        if (!node) { close(*cfd); free(cfd); continue; }

        node->fd = *cfd;
        SLIST_INSERT_HEAD(&head, node, entries);
        pthread_create(&node->tid, NULL, client_handler, cfd);
    }

    pthread_join(timer_tid, NULL);

    while (!SLIST_EMPTY(&head)) {
        struct client_thread *n = SLIST_FIRST(&head);
        SLIST_REMOVE_HEAD(&head, entries);
        pthread_join(n->tid, NULL);
        free(n);
    }

    return 0;
}

