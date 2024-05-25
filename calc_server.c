#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

void *client_proc(void *);

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8000);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind() failed");
        return 1;
    }

    if (listen(listener, 10)) {
        perror("listen() failed");
        return 1;
    }

    while (1) {
        printf("Waiting for new client\n");
        int client = accept(listener, NULL, NULL);
        printf("New client accepted, client = %d\n", client);
        
        pthread_t tid;
        pthread_create(&tid, NULL, client_proc, &client);
        pthread_detach(tid);
    }

    return 0;
}

void *client_proc(void *arg) {
    int client = *(int *)arg;
    char buf[2048];

    int ret = recv(client, buf, sizeof(buf), 0);
    if (ret <= 0) {
        close(client);
        pthread_exit(NULL);
    }

    buf[ret] = 0;
    printf("Received from %d: %s\n", client, buf);

    char response[4096];
    if (strncmp(buf, "GET / ", 6) == 0) {
        // Tra ve trang web Hello World
        strcpy(buf, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>Type parameters /calc?a=..&b=..&cmd=..</h1>");
        send(client, buf, strlen(buf), 0);
    } else if (strncmp(buf, "GET /calc?", 10) == 0 || strncmp(buf, "POST /calc", 10) == 0) {
        char *a_str = strstr(buf, "a=") + 2;
        char *b_str = strstr(buf, "&b=") + 3;
        char *cmd_str = strstr(buf, "&cmd=") + 5;

        a_str = strtok(a_str, "&");
        b_str = strtok(b_str, "&");
        cmd_str = strtok(cmd_str, " ");

        double a = atof(a_str);
        double b = atof(b_str);
        char *cmd = cmd_str;

        double result = 0;
        if (strcmp(cmd, "add") == 0) {
            result = a + b;
        } else if (strcmp(cmd, "sub") == 0) {
            result = a - b;
        } else if (strcmp(cmd, "mul") == 0) {
            result = a * b;
        } else if (strcmp(cmd, "div") == 0) {
            result = a / b;
        }

        snprintf(response, sizeof(response),
            "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
            "<html><body>"
            "<h1>Result: %.2f</h1>"
            "</body></html>", result);
        send(client, response, strlen(response), 0);
    }
    close(client);
    pthread_exit(NULL);
}
