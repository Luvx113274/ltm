#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

void *client_proc(void *);

int get_index(int client);
int remove_client(int client, int *client_sockets, char **client_names, int *num_clients, char **client_topics);
int check_join(int client);
int process_join(int client, char *buf);
int process_msg(int client, char *buf);
int process_pmsg(int client, char *buf);
int process_op(int client, char *buf);
int process_kick(int client, char *buf);
int process_topic(int client, char *buf);
int process_quit(int client, char *buf);

int client_sockets[1024];
char *client_names[1024];
char *client_topics[1024]; // Thêm mảng lưu trữ phòng chat của từng client
int num_clients = 0;

char *operator = NULL; // Lưu trữ operator
char topic[256]; // Lưu trữ topic hiện tại

int main() {
    // Tạo socket cho kết nối
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1) {
        perror("socket() failed");
        return 1;
    }

    // Khai báo địa chỉ server
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8000);

    // Gắn socket với cấu trúc địa chỉ
    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind() failed");
        return 1;
    }

    // Chuyển socket sang trạng thái chờ kết nối
    if (listen(listener, 5)) {
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

int remove_client(int client, int *client_sockets, char **client_names, int *num_clients, char **client_topics) {
    int i = 0;
    for (; i < *num_clients; i++)
        if (client_sockets[i] == client)
            break;
    
    if (i < *num_clients) {
        if (i < *num_clients - 1) {
            client_sockets[i] = client_sockets[*num_clients - 1];
            strcpy(client_names[i], client_names[*num_clients - 1]);
            strcpy(client_topics[i], client_topics[*num_clients - 1]);
        }

        free(client_names[*num_clients - 1]);
        free(client_topics[*num_clients - 1]);
        *num_clients -= 1;
    }
    return 0;
}

void *client_proc(void *arg) {
    int client = *(int *)arg;
    char buf[256];

    // Nhận dữ liệu từ client
    while (1) {
        int ret = recv(client, buf, sizeof(buf), 0);
        if (ret <= 0) {
            remove_client(client, client_sockets, client_names, &num_clients, client_topics);
            break;
        }
            
        buf[ret] = 0;
        printf("Received from %d: %s\n", client, buf);

        if (strncmp(buf, "JOIN ", 5) == 0)
            process_join(client, buf);
        else if (strncmp(buf, "MSG ", 4) == 0)
            process_msg(client, buf);
        else if (strncmp(buf, "PMSG ", 5) == 0)
            process_pmsg(client, buf);
        else if (strncmp(buf, "OP ", 3) == 0)
            process_op(client, buf);
        else if (strncmp(buf, "KICK ", 5) == 0)
            process_kick(client, buf);
        else if (strncmp(buf, "TOPIC ", 6) == 0)
            process_topic(client, buf);
        else if (strncmp(buf, "QUIT", 4) == 0)
            process_quit(client, buf);
        else {
            char *msg = "999 Unknown command\n";
            send(client, msg, strlen(msg), 0);
        }
    }

    close(client);
}

int check_join(int client) {
    int i = 0;
    for (; i < num_clients; i++)
        if (client_sockets[i] == client)
            break;
    return i < num_clients;
}

int get_index(int client) {
    int i = 0;
    for (; i < num_clients; i++)
        if (client_sockets[i] == client)
            break;
    return i;
}

int process_join(int client, char *buf) {
    if (!check_join(client)) {
        // Chưa đăng nhập
        char cmd[16], id[32], tmp[32];
        int n = sscanf(buf, "%s %s %s", cmd, id, tmp);
        if (n == 2) {
            // Kiểm tra tính hợp lệ của id
            // id chỉ chứa ký tự chữ thường và chữ số
            int k = 0;
            for (; k < strlen(id); k++)
                if (id[k] < '0' || id[k] > 'z' || (id[k] > '9' && id[k] < 'a')) 
                    break;
            if (k < strlen(id)) {
                // id chứa ký tự không hợp lệ
                char *msg = "201 Invalid nickname\n";
                send(client, msg, strlen(msg), 0);
            } else {
                // Kiểm tra id đã tồn tại chưa
                k = 0;
                for (; k < num_clients; k++)
                    if (strcmp(client_names[k], id) == 0)
                        break;
                if (k < num_clients) {
                    // id đã tồn tại
                    char *msg = "200 Nickname in use\n";
                    send(client, msg, strlen(msg), 0);
                } else {
                    // id chưa tồn tại
                    char *msg = "100 OK\n";
                    send(client, msg, strlen(msg), 0);

                    // Chuyển client sang trạng thái đăng nhập
                    client_sockets[num_clients] = client;
                    client_names[num_clients] = malloc(strlen(id) + 1);
                    memcpy(client_names[num_clients], id, strlen(id) + 1);
                    client_topics[num_clients] = NULL; // Chưa tham gia phòng chat nào
                    num_clients++;

                    // Gửi thông điệp cho các client khác
                    for (int k = 0; k < num_clients; k++)
                        if (client_sockets[k] != client) {
                            char msg[512];
                            sprintf(msg, "JOIN %s\n", id);
                            send(client_sockets[k], msg, strlen(msg), 0);
                        }
                }
            }
        } else {
            char *msg = "999 Unknown error\n";
            send(client, msg, strlen(msg), 0);
        }
    } else {
        char *msg = "888 Wrong state\n";
        send(client, msg, strlen(msg), 0);
    }

    return 0;
}

int process_msg(int client, char *buf) {
    if (check_join(client)) {
        int idx = get_index(client);
        // Chuyển tiếp tin nhắn cho các client đã tham gia cùng phòng chat
        for (int k = 0; k < num_clients; k++)
            if (client_sockets[k] != client && client_topics[k] && strcmp(client_topics[k], client_topics[idx]) == 0) {
                char msg[512];
                sprintf(msg, "MSG %s %s\n", client_names[idx], buf + 4);
                send(client_sockets[k], msg, strlen(msg), 0);
            }

        char *msg = "100 OK\n";
        send(client, msg, strlen(msg), 0);
    } else {
        char *msg = "999 Unknown error\n";
        send(client, msg, strlen(msg), 0);
    }
    return 0;
}

int process_pmsg(int client, char *buf) {
    if (check_join(client)) {
        int idx = get_index(client);

        char receiver[32];
        sscanf(buf + 5, "%s", receiver);

        // Chuyển tiếp tin nhắn cho một client
        int k = 0; 
        for (; k < num_clients; k++)
            if (strcmp(client_names[k], receiver) == 0)     
                break;
        
        if (k < num_clients) {
            // Tìm thấy người nhận
            char msg[512];
            sprintf(msg, "PMSG %s %s\n", client_names[idx], buf + strlen(receiver) + 6);
            send(client_sockets[k], msg, strlen(msg), 0);

            char *response = "100 OK\n";
            send(client, response, strlen(response), 0);
        } else {
            // Không tìm thấy người nhận
            char *msg = "202 Unknown nickname\n";
            send(client, msg, strlen(msg), 0);
        }
    } else {
        char *msg = "999 Unknown error\n";
        send(client, msg, strlen(msg), 0);
    }
    return 0;
}

int process_op(int client, char *buf) {
    if (check_join(client)) {
        int idx = get_index(client);
        char cmd[16], id[32];
        int n = sscanf(buf, "%s %s", cmd, id);
        if (n == 2 && operator == NULL) {
            operator = strdup(id);
            char msg[512];
            sprintf(msg, "OP %s\n", id);
            for (int k = 0; k < num_clients; k++)
                send(client_sockets[k], msg, strlen(msg), 0);
            char *response = "100 OK\n";
            send(client, response, strlen(response), 0);
        } else {
            char *msg = "999 Failed to set operator\n";
            send(client, msg, strlen(msg), 0);
        }
    } else {
        char *msg = "999 Unknown error\n";
        send(client, msg, strlen(msg), 0);
    }
    return 0;
}

int process_kick(int client, char *buf) {
    if (check_join(client) && operator && strcmp(client_names[get_index(client)], operator) == 0) {
        char cmd[16], id[32];
        sscanf(buf, "%s %s", cmd, id);
        int k = 0; 
        for (; k < num_clients; k++)
            if (strcmp(client_names[k], id) == 0)
                break;

        if (k < num_clients) {
            char msg[512];
            sprintf(msg, "KICK %s\n", id);
            send(client_sockets[k], msg, strlen(msg), 0);

            close(client_sockets[k]);
            remove_client(client_sockets[k], client_sockets, client_names, &num_clients, client_topics);

            char *response = "100 OK\n";
            send(client, response, strlen(response), 0);
        } else {
            char *msg = "202 Unknown nickname\n";
            send(client, msg, strlen(msg), 0);
        }
    } else {
        char *msg = "999 Permission denied\n";
        send(client, msg, strlen(msg), 0);
    }
    return 0;
}

int process_topic(int client, char *buf) {
    int idx = get_index(client);
    if (check_join(client)) {
        if (operator && strcmp(client_names[idx], operator) == 0) {
            // Nếu người gửi là operator, kiểm tra và tạo topic mới nếu chưa tồn tại
            strncpy(topic, buf + 6, sizeof(topic) - 1);
            topic[sizeof(topic) - 1] = '\0';
            
            char msg[512];
            sprintf(msg, "TOPIC %s\n", topic);
            for (int k = 0; k < num_clients; k++)
                send(client_sockets[k], msg, strlen(msg), 0);

            char *response = "100 OK\n";
            send(client, response, strlen(response), 0);
        } else {
            // Nếu người gửi là user, tham gia vào topic đã tồn tại
            if (strlen(topic) > 0 && strncmp(buf + 6, topic, strlen(topic)) == 0) {
                client_topics[idx] = strdup(topic);
                char *response = "100 OK\n";
                send(client, response, strlen(response), 0);
            } else {
                char *msg = "999 Topic not found or permission denied\n";
                send(client, msg, strlen(msg), 0);
            }
        }
    } else {
        char *msg = "999 Unknown error\n";
        send(client, msg, strlen(msg), 0);
    }
    return 0;
}

int process_quit(int client, char *buf) {
    if (check_join(client)) {
        int idx = get_index(client);
        char msg[512];
        sprintf(msg, "QUIT %s\n", client_names[idx]);
        for (int k = 0; k < num_clients; k++)
            if (client_sockets[k] != client)
                send(client_sockets[k], msg, strlen(msg), 0);

        close(client);
        remove_client(client, client_sockets, client_names, &num_clients, client_topics);

        if (operator && strcmp(client_names[idx], operator) == 0) {
            free(operator);
            operator = NULL;
        }
    } else {
        char *msg = "999 Unknown error\n";
        send(client, msg, strlen(msg), 0);
    }
    return 0;
}
