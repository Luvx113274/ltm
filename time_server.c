#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>

char* get_current_time(char* format) {
    time_t now;
    struct tm *tm_info;
    char *time_str = (char*)malloc(256 * sizeof(char));

    time(&now);
    tm_info = localtime(&now);

    if(strncmp(format, "dd/mm/yyyy", 10) == 0) {
        strftime(time_str, 256, "%d/%m/%Y", tm_info);
    } else if(strncmp(format, "dd/mm/yy", 8) == 0) {
        strftime(time_str, 256, "%d/%m/%y", tm_info);
    } else if(strncmp(format, "mm/dd/yyyy", 10) == 0) {
        strftime(time_str, 256, "%m/%d/%Y", tm_info);
    } else if(strncmp(format, "mm/dd/yy", 8) == 0) {
        strftime(time_str, 256, "%m/%d/%y", tm_info);
    } else {
        strftime(time_str, 256, "Invalid format!", tm_info);
    }
    return time_str;
}

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
    addr.sin_port = htons(9000);

    // Gán socket với cấu trúc địa chỉ
    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind() failed");
        return 1;
    }

    // Chuyển socket sang trạng thái chờ kết nối
    if (listen(listener, 5)) {
        perror("listen() failed");
        return 1;
    }

    // Fork các tiến trình con
    for (int i = 0; i < 8; i++) {
        if (fork() == 0) {
            char buf[256];
            while (1) {
                int client = accept(listener, NULL, NULL);

                int ret = recv(client, buf, sizeof(buf), 0);
                if (ret <= 0) {
                    close(client);
                    continue;
                }

                if (ret < sizeof(buf))
                    buf[ret] = '\0';

                // Kiểm tra lệnh client và trả về thời gian tương ứng
                if (strncmp(buf, "GET_TIME", 8) == 0) {
                    char* format = buf + 9;
                    printf("Format: %s\n", format);
                    char* time_str;

                    time_str = get_current_time(format);

                    send(client, time_str, strlen(time_str), 0);
                    free(time_str);
                } else {
                    char* error_msg = "Invalid command!";
                    send(client, error_msg, strlen(error_msg), 0);
                }

                close(client);
            }
            exit(0);
        }
    }

    // Chờ nhập ký tự từ bàn phím để kết thúc chương trình
    getchar();

    // Đóng socket lắng nghe và kết thúc tiến trình cha
    close(listener);

    return 0;
}
