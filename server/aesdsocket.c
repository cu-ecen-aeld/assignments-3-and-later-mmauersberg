/**
 * Socket server
 * Author: Martin Mauersberg
 * Date: 10/12/2023
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <syslog.h>
#include <signal.h>

#include <pthread.h>

#define SOCKET_PORT "9000"
#define SOCKET_BACKLOG 10
#define BUF_SIZE 1024
#define OUTFILE "/var/tmp/aesdsocketdata"

volatile sig_atomic_t done = 0;

/************************************************************************************
 * ----------------------  handlers for SIGINT and SIGTERM ----------------------
 * **********************************************************************************/
void sigterm_handler(int s) {
    printf("Caught SIGTERM, exiting\n");
    syslog(LOG_USER,"Caught signal, exiting\n");
    done = 1;
}

void sigint_handler(int s) {
    printf("Caught SIGINT, exiting\n");
    syslog(LOG_USER,"Caught signal, exiting\n");
    done = 1;
}

void sigchld_handler(int s) {
    int saved_errno = errno;
    while (waitpid(-1,NULL, WNOHANG) > 0) {}
    errno = saved_errno;
}

/* get sockaddr for ipv4 or ipv6 */
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET){
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/************************************************************************************
 * ----------------------  connection handler  ----------------------
 * **********************************************************************************/
void *connection_handler (void *socket_desc)
{
    int sock = *(int*)socket_desc;

    ssize_t nread;
    char buf[BUF_SIZE] = {0};

    FILE *fp = fopen(OUTFILE, "a+b");
    if (fp == NULL) {
        perror("fopen");
        exit(-1);
    }
    do {
        nread = recv(sock, &buf, BUF_SIZE, 0);
        if (nread == -1) {
            perror("recv");
            exit(-1);
        } else {
            fwrite(&buf, 1, nread, fp);
            fseek(fp, 0, SEEK_SET);
            for (int i = 0; i < nread; i++){
                if (buf[i] == '\n') {
                    // printf("got newline\n");
                    while (1){
                        char c = fgetc(fp);
                        if (c == EOF){
                            break;
                        }
                        send(sock, &c, 1, 0);
                    }
                }
            }
        }
    } while(nread != 0 && done ==0);
    fclose(fp);

    free(socket_desc);
    printf("closed Connection\n");
    syslog(LOG_USER, "Closed connection\n");

    return 0;
}

/************************************************************************************
 * ----------------------  MAIN ----------------------
 * **********************************************************************************/
int main(int argc, char *argv[]){
    if (argc > 1){
        // if we have arguments, check for -d
        if (strncmp(argv[1], "-d", strlen("-d")) == 0){
            // if daemon mode is specified, fork the process and exit parent
            pid_t child_pid = fork();
            if (child_pid == -1) { perror("fork"); exit(-1);}
            if (child_pid != 0) {
                // exit parent
                exit(0);
            }
        }else{
            fprintf(stderr,"unknown arguments - ignored\n");
        }
    }

    /************************************************************************************
     * INITIALIZATION
     * **********************************************************************************/

    /* Initialize SIGTERM handler */
    struct sigaction sa_sigterm;
    memset(&sa_sigterm, 0, sizeof(sa_sigterm));
    sa_sigterm.sa_handler = sigterm_handler;
    sigemptyset(&sa_sigterm.sa_mask);
    if (sigaction( SIGTERM, &sa_sigterm, NULL) == -1){ //
        exit(-1);
    }

    /* Intialize SIGINT handler */
    struct sigaction sa_sigint;
    memset(&sa_sigint, 0, sizeof(sa_sigint));
    sa_sigint.sa_handler = sigint_handler;
    sigemptyset(&sa_sigint.sa_mask);
    if (sigaction( SIGINT, &sa_sigint, NULL) == -1){ // 
        exit(-1);
    }

    /************************************************************************************
     * OPEN SOCKET
     * **********************************************************************************/

    int sockfd, new_fd;
    int yes = 1;
    socklen_t sin_size;
    struct sockaddr_storage sin_addr;
    char s[INET6_ADDRSTRLEN];
    int retval;

    /* set hints (connection parameters) */
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;        // can be IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;    // we need a stream
    hints.ai_flags = AI_PASSIVE;        // fill in IP for me


    /* get server info and store in servinfo */
    struct addrinfo *servinfo;
    retval = getaddrinfo(NULL, SOCKET_PORT, &hints, &servinfo);
    if (retval != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(retval));
        syslog(LOG_ERR, "getaddrinfo: %s\n", gai_strerror(retval));
        exit(-1);
    }

    /* loop through all servinfo results and bind to the first valid one */
    struct addrinfo  *p;
    for (p = servinfo; p != NULL; p = p -> ai_next){

        /* read address and ip version per addressinfo record */
        void *addr;
        char *ipver;
        char ipstr[INET6_ADDRSTRLEN];

        if (p->ai_family == AF_INET) {
            struct sockaddr_in *ipv4 = (struct sockaddr_in*)p->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
        } else {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6*)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
        }
        // convert the IP to a string and print it
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        printf("Listening on %s: %s \n", ipver, ipstr);


        // create socket and return socket file descriptor
        sockfd = socket (p->ai_family, p -> ai_socktype, p -> ai_protocol);
        if (sockfd == -1) {
            syslog(LOG_ERR, "failed to create socket for %s:%s\n", ipver, ipstr);
            continue;
        }

        // set socket options
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
            close(sockfd);
            syslog(LOG_ERR, "failed to set socket options for %s:%s\n", ipver, ipstr);
            exit (-1);
        }

        // bind socket to port
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            syslog(LOG_ERR, "failed to bind socket to %s:%s\n", ipver, ipstr);
            continue;
        }

        break;
    }

    // free servinfo allocation
    freeaddrinfo(servinfo);

    /************************************************************************************
     * WAIT FOR CONNECTION AND HANDLE CONNECTION IN THREAD
     * **********************************************************************************/

    // check if we have a port bound
    if (p == NULL){
        syslog(LOG_ERR, "server: failed to bind\n");
        exit(-1);
    }

    // Start listening for a connection.
    if (listen(sockfd, SOCKET_BACKLOG) == -1){
        syslog(LOG_ERR, "server: failed to listen\n");
        exit(-1);
    }

    printf("server: waiting for connections...\n");
    syslog(LOG_USER, "waiting for connections...\n");

    while(done == 0){
        sin_size = sizeof(sin_addr);

        // Wait for a connection.
        new_fd = accept(sockfd, (struct sockaddr*)&sin_addr, &sin_size);
        if (new_fd == -1){
            syslog(LOG_ERR, "server: failed to accept connection\n");
            continue;
        }

        // get address for incoming connection
        inet_ntop(sin_addr.ss_family, get_in_addr((struct sockaddr*)&sin_addr), s, sizeof(s));
        syslog(LOG_USER, "Accepted connection from %s\n", s);

        // initialize thread to handle connection
        pthread_t connection_thread;
        int *socket_desc = malloc(1);
        *socket_desc = new_fd;
        if (pthread_create(&connection_thread, NULL, connection_handler, (void*) socket_desc)) {
            syslog(LOG_ERR,"could not create thread for connection from %s\n", s);
        }
     
    }

    // Cleanup.
    remove(OUTFILE);
    close(sockfd);

    return 0;
}

