#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <limits.h>

#define PORT "8000"
#define BACKLOG 10
#define BUFFER_SIZE 1024

// This is the thread function to handle the request
void *handle_request(void *arg) {
    int client_fd = *(int *)arg;
    free(arg); 

    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));

    //This is how we read the HTTP request
    ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0) {
        perror("read");
        close(client_fd);
        return NULL;
    }

    //This is how we parse the GET request
    char *method = strtok(buffer, " ");
    char *path = strtok(NULL, " ");
    if (!method || !path || strcmp(method, "GET") != 0) {
        fprintf(stderr, "Invalid request\n");
        close(client_fd);
        return NULL;
    }

    //this is how we remove the leading '/' from the path
    if (path[0] == '/') {
        path++;
    }

    //This is how we construct the full path to the file
    char full_path[PATH_MAX];
    snprintf(full_path, sizeof(full_path), "%s/%s", ".", path); 

    //this is how we open the file
    int file_fd = open(full_path, O_RDONLY);
    if (file_fd == -1) {
        //if the file is not found: send a 404 response
        const char *not_found_response = "HTTP/1.1 404 Not Found\r\n"
                                         "Content-Length: 13\r\n"
                                         "\r\n"
                                         "404 Not Found";
        write(client_fd, not_found_response, strlen(not_found_response));
    } else {
        
        struct stat file_stat;
        if (stat(full_path, &file_stat) == -1) {
            perror("stat");
            close(file_fd);
            close(client_fd);
            return NULL;
        }

        //This is what sends a 200 OK response
        char header[BUFFER_SIZE];
        snprintf(header, sizeof(header), "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n", file_stat.st_size);
        write(client_fd, header, strlen(header));

        //this is what sends the file contents
        char file_buffer[BUFFER_SIZE];
        ssize_t bytes;
        while ((bytes = read(file_fd, file_buffer, sizeof(file_buffer))) > 0) {
            write(client_fd, file_buffer, bytes);
        }
        close(file_fd);
    }

    
    close(client_fd);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <root_directory>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *root_dir = argv[1];

    //This is where we change the working directory to the root directory
    if (chdir(root_dir) == -1) {
        perror("chdir");
        return EXIT_FAILURE;
    }

    //This isn where wet up the socket
    struct addrinfo hints, *res;
    int server_fd;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;       
    hints.ai_socktype = SOCK_STREAM; 
    hints.ai_flags = AI_PASSIVE;     

    if (getaddrinfo(NULL, PORT, &hints, &res) != 0) {
        perror("getaddrinfo");
        return EXIT_FAILURE;
    }

    server_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (server_fd == -1) {
        perror("socket");
        freeaddrinfo(res);
        return EXIT_FAILURE;
    }

    if (bind(server_fd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("bind");
        close(server_fd);
        freeaddrinfo(res);
        return EXIT_FAILURE;
    }

    freeaddrinfo(res);

    if (listen(server_fd, BACKLOG) == -1) {
        perror("listen");
        close(server_fd);
        return EXIT_FAILURE;
    }

    printf("Server is listening on port %s...\n", PORT);

    //this is the main loop that accepts and handles connections
    while (1) {
        struct sockaddr_storage client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int *client_fd = malloc(sizeof(int));
        if (!client_fd) {
            perror("malloc");
            continue;
        }

        *client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (*client_fd == -1) {
            perror("accept");
            free(client_fd);
            continue;
        }

        //ths is where we create a new thread to handle the request
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_request, client_fd) != 0) {
            perror("pthread_create");
            close(*client_fd);
            free(client_fd);
            continue;
        }

        pthread_detach(thread_id); 
    }

    close(server_fd);
    return EXIT_SUCCESS;
}
