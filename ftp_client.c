#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>

int client;

unsigned short send_pasv();
int send_list();
int download_file(char *remote_file);
int upload_file(char *local_file);
int rename_file(char *cur_file, char *new_file);
int delete_file(char *filename);
int print_working_dir();
int change_working_dir(char *dirname);
int make_dir(char *dirname);
int remove_dir(char *dirname);

int main() 
{
    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("172.20.190.212");
    addr.sin_port = htons(21);

    int ret = connect(client, (struct sockaddr *) &addr, sizeof(addr));
    if (ret == -1) {
        perror("connect() failed");
        return 1;
    }

    char buf[2048];

    // Nhan xau chao
    ret = recv(client, buf, sizeof(buf), 0);
    if (ret <= 0) {
        close(client);
        return 1;
    }

    buf[ret] = 0;
    puts(buf);

    // Dang nhap
    char user[32], pass[32];
    printf("Nhap username: ");
    fgets(user, sizeof(user), stdin);
    printf("Nhap password: ");
    fgets(pass, sizeof(pass), stdin);

    // Xoa ky tu xuong trong
    user[strlen(user) - 1] = 0;
    pass[strlen(pass) - 1] = 0;

    // Gui lenh USER
    sprintf(buf, "USER %s\r\n", user);
    send(client, buf, strlen(buf), 0);

    ret = recv(client, buf, sizeof(buf), 0);
    if (ret <= 0) {
        close(client);
        return 1;
    }

    buf[ret] = 0;
    puts(buf);

    // Gui lenh PASS
    sprintf(buf, "PASS %s\r\n", pass);
    send(client, buf, strlen(buf), 0);

    ret = recv(client, buf, sizeof(buf), 0);
    if (ret <= 0) {
        close(client);
        return 1;
    }

    buf[ret] = 0;
    puts(buf);

    // Kiem tra dang nhap thanh cong
    if (strncmp(buf, "230 ", 4) == 0) {
        puts("Dang nhap thanh cong.");
    } else {
        puts("Dang nhap that bai.");
        close(client);
        return 1;
    }

    int choice;
    char filename[256], new_filename[256], dirname[256];

    while (1) {
        printf("Hay chon chuc nang de thuc hien:\n");
        printf("1. In noi dung thu muc hien tai\n");
        printf("2. Doi thu muc hien tai\n");
        printf("3. Tao thu muc\n");
        printf("4. Xoa thu muc\n");
        printf("5. Download file\n");
        printf("6. Upload file\n");
        printf("7. Doi ten file\n");
        printf("8. Xoa file\n");
        printf("0. Thoat va ngat ket noi\n");
        printf("Lua chon: ");
        scanf("%d", &choice);
        getchar(); // Xóa ký tự newline sau khi nhập số

        switch (choice) {
            case 1:
                send_list();
                break;
            case 2:
                printf("Nhap ten thu muc de chuyen den: ");
                fgets(dirname, sizeof(dirname), stdin);
                dirname[strlen(dirname) - 1] = 0;
                change_working_dir(dirname);
                break;
            case 3:
                printf("Nhap ten thu muc de tao: ");
                fgets(dirname, sizeof(dirname), stdin);
                dirname[strlen(dirname) - 1] = 0;
                make_dir(dirname);
                break;
            case 4:
                printf("Nhap ten thu muc de xoa: ");
                fgets(dirname, sizeof(dirname), stdin);
                dirname[strlen(dirname) - 1] = 0;
                remove_dir(dirname);
                break;
            case 5:
                printf("Nhap ten file de download: ");
                fgets(filename, sizeof(filename), stdin);
                filename[strlen(filename) - 1] = 0;
                download_file(filename);
                break;
            case 6:
                printf("Nhap ten file de upload: ");
                fgets(filename, sizeof(filename), stdin);
                filename[strlen(filename) - 1] = 0;
                upload_file(filename);
                break;
            case 7:
                printf("Nhap ten file hien tai: ");
                fgets(filename, sizeof(filename), stdin);
                filename[strlen(filename) - 1] = 0;
                printf("Nhap ten file moi: ");
                fgets(new_filename, sizeof(new_filename), stdin);
                new_filename[strlen(new_filename) - 1] = 0;
                rename_file(filename, new_filename);
                break;
            case 8:
                printf("Nhap ten file de xoa: ");
                fgets(filename, sizeof(filename), stdin);
                filename[strlen(filename) - 1] = 0;
                delete_file(filename);
                break;
            case 0:
                close(client);
                return 0;
            default:
                printf("Lua chon khong hop le. Vui long thu lai.\n");
        }
    }
}

unsigned short send_pasv() {
    char buf[2048];

    send(client, "PASV\r\n", 6, 0);

    int ret = recv(client, buf, sizeof(buf), 0);
    if (ret <= 0) {
        close(client);
        exit(1);
    }

    buf[ret] = 0;
    // puts(buf);

    char *pos = strchr(buf, '(');
    int i1 = atoi(strtok(pos, "(),"));
    int i2 = atoi(strtok(NULL, "(),"));
    int i3 = atoi(strtok(NULL, "(),"));
    int i4 = atoi(strtok(NULL, "(),"));
    int p1 = atoi(strtok(NULL, "(),"));
    int p2 = atoi(strtok(NULL, "(),"));

    return p1 * 256 + p2;
}

int send_list() {
    // Gui lenh PASV
    unsigned short port = send_pasv();
    printf("Port: %d\n", port);

    // Mo ket noi du lieu 
    int client_data = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in addr_data;
    addr_data.sin_family = AF_INET;
    addr_data.sin_addr.s_addr = inet_addr("172.20.190.212");
    addr_data.sin_port = htons(port);

    int ret = connect(client_data, (struct sockaddr *) &addr_data, sizeof(addr_data));
    if (ret == -1) {
        perror("connect() failed");
        return 1;
    }

    // Gui lenh LIST
    send(client, "LIST\r\n", 6, 0);

    char buf[2048];

    ret = recv(client, buf, sizeof(buf), 0);
    if (ret <= 0) {
        close(client);
        return 1;
    }

    buf[ret] = 0;
    puts(buf);

    // Nhan du lieu tren kenh du lieu
    // In ra man hinh
    while (1) {
        ret = recv(client_data, buf, sizeof(buf) - 1, 0);
        if (ret <= 0) {
            close(client_data);
            break;
        }

        buf[ret] = 0;
        printf("%s", buf);
    }

    printf("\n");

    ret = recv(client, buf, sizeof(buf), 0);
    if (ret <= 0) {
        close(client);
        return 1;
    }

    buf[ret] = 0;
    puts(buf);
    return 0;
}

int download_file(char *remote_file) {
    unsigned short port = send_pasv();
    printf("Port: %d\n", port);

    // Mo ket noi du lieu 
    int client_data = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in addr_data;
    addr_data.sin_family = AF_INET;
    addr_data.sin_addr.s_addr = inet_addr("172.20.190.212");
    addr_data.sin_port = htons(port);

    int ret = connect(client_data, (struct sockaddr *) &addr_data, sizeof(addr_data));
    if (ret == -1) {
        perror("connect() failed");
        return 1;
    }

    char buf[2048];

    // Gui lenh RETR
    sprintf(buf, "RETR %s\r\n", remote_file);
    send(client, buf, strlen(buf), 0);

    ret = recv(client, buf, sizeof(buf), 0);
    if (ret <= 0) {
        close(client);
        return 1;
    }

    buf[ret] = 0;
    puts(buf);

    // Nhan noi dung file
    FILE *f = fopen(remote_file, "wb");
    while (1) {
        ret = recv(client_data, buf, sizeof(buf), 0);
        if (ret <= 0)
            break;
        fwrite(buf, 1, ret, f);
    }
    close(client_data);
    fclose(f);

    ret = recv(client, buf, sizeof(buf), 0);
    if (ret <= 0) {
        close(client);
        return 1;
    }

    buf[ret] = 0;
    puts(buf);
    return 0;
}

int upload_file(char *local_file) {
    unsigned short port = send_pasv();
    printf("Port: %d\n", port);

    // Mo ket noi du lieu 
    int client_data = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in addr_data;
    addr_data.sin_family = AF_INET;
    addr_data.sin_addr.s_addr = inet_addr("172.20.190.212");
    addr_data.sin_port = htons(port);

    int ret = connect(client_data, (struct sockaddr *) &addr_data, sizeof(addr_data));
    if (ret == -1) {
        perror("connect() failed");
        return 1;
    }

    char buf[2048];

    // Gui lenh STOR
    sprintf(buf, "STOR %s\r\n", local_file);
    send(client, buf, strlen(buf), 0);

    ret = recv(client, buf, sizeof(buf), 0);
    if (ret <= 0) {
        close(client);
        return 1;
    }

    buf[ret] = 0;
    puts(buf);

    // Gui noi dung file
    FILE *f = fopen(local_file, "rb");
    while (1) {
        ret = fread(buf, 1, sizeof(buf), f);
        if (ret <= 0)
            break;
        send(client_data, buf, ret, 0);
    }
    close(client_data);
    fclose(f);

    ret = recv(client, buf, sizeof(buf), 0);
    if (ret <= 0) {
        close(client);
        return 1;
    }

    buf[ret] = 0;
    puts(buf);
    return 0;
}

int rename_file(char *cur_file, char *new_file) {
    char buf[2048];

    // Gui lenh RNFR
    sprintf(buf, "RNFR %s\r\n", cur_file);
    send(client, buf, strlen(buf), 0);

    int ret = recv(client, buf, sizeof(buf), 0);
    if (ret <= 0) {
        close(client);
        return 1;
    }

    buf[ret] = 0;
    puts(buf);

    // Gui lenh RNTO
    sprintf(buf, "RNTO %s\r\n", new_file);
    send(client, buf, strlen(buf), 0);

    ret = recv(client, buf, sizeof(buf), 0);
    if (ret <= 0) {
        close(client);
        return 1;
    }

    buf[ret] = 0;
    puts(buf);
    return 0;
}

int delete_file(char *filename) {
    char buf[2048];

    // Gui lenh DELE
    sprintf(buf, "DELE %s\r\n", filename);
    send(client, buf, strlen(buf), 0);

    int ret = recv(client, buf, sizeof(buf), 0);
    if (ret <= 0) {
        close(client);
        return 1;
    }

    buf[ret] = 0;
    puts(buf);
    return 0;
}

int print_working_dir() {
    char buf[2048];

    // Gui lenh PWD
    send(client, "PWD\r\n", 5, 0);

    int ret = recv(client, buf, sizeof(buf), 0);
    if (ret <= 0) {
        close(client);
        return 1;
    }

    buf[ret] = 0;
    puts(buf);
    return 0;
}

int change_working_dir(char *dirname) {
    char buf[2048];

    // Gui lenh CWD
    sprintf(buf, "CWD %s\r\n", dirname);
    send(client, buf, strlen(buf), 0);

    int ret = recv(client, buf, sizeof(buf), 0);
    if (ret <= 0) {
        close(client);
        return 1;
    }

    buf[ret] = 0;
    puts(buf);
    return 0;
}

int make_dir(char *dirname) {
    char buf[2048];

    // Gui lenh MKD
    sprintf(buf, "MKD %s\r\n", dirname);
    send(client, buf, strlen(buf), 0);

    int ret = recv(client, buf, sizeof(buf), 0);
    if (ret <= 0) {
        close(client);
        return 1;
    }

    buf[ret] = 0;
    puts(buf);
    return 0;
}

int remove_dir(char *dirname) {
    char buf[2048];

    // Gui lenh RMD
    sprintf(buf, "RMD %s\r\n", dirname);
    send(client, buf, strlen(buf), 0);

    int ret = recv(client, buf, sizeof(buf), 0);
    if (ret <= 0) {
        close(client);
        return 1;
    }

    buf[ret] = 0;
    puts(buf);
    return 0;
}
