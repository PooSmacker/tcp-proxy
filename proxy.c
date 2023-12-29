#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <json-c/json.h>
#include <netdb.h>

#define BUFFER_SIZE 4096

struct ProxyConfig {
    int frontend_port;
    char backend_ip[INET_ADDRSTRLEN];
    int backend_port;
    char optional_note[256];
};

void load_config(const char *config_file, struct ProxyConfig *config) {
    FILE *file = fopen(config_file, "r");
    if (!file) {
        perror("Error opening config file");
        exit(EXIT_FAILURE);
    }

    char buffer[4096];
    struct json_object *json_obj = json_object_from_file(config_file);
    if (!json_obj) {
        fprintf(stderr, "Error parsing JSON from config file\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    struct json_object *settings_obj, *proxied_host_obj, *general_settings_obj;

    if (json_object_object_get_ex(json_obj, "settings", &settings_obj)) {
        if (json_object_object_get_ex(settings_obj, "frontend_port", &settings_obj)) {
            config->frontend_port = json_object_get_int(settings_obj);
        }
    }

    if (json_object_object_get_ex(json_obj, "proxied_host", &proxied_host_obj)) {
        if (json_object_object_get_ex(proxied_host_obj, "general_settings", &general_settings_obj)) {
            if (json_object_object_get_ex(general_settings_obj, "backend_ip", &settings_obj)) {
                strcpy(config->backend_ip, json_object_get_string(settings_obj));
            }
            if (json_object_object_get_ex(general_settings_obj, "backend_port", &settings_obj)) {
                config->backend_port = json_object_get_int(settings_obj);
            }
            if (json_object_object_get_ex(general_settings_obj, "optional_note", &settings_obj)) {
                strcpy(config->optional_note, json_object_get_string(settings_obj));
            }
        }
    }

    fclose(file);
}

void get_local_ip(char *ip) {
    char buffer[INET_ADDRSTRLEN];
    struct hostent *host_entry;

    gethostname(buffer, sizeof(buffer));

    host_entry = gethostbyname(buffer);

    if (host_entry == NULL) {
        perror("Error getting local host information");
        exit(EXIT_FAILURE);
    }

    strcpy(ip, inet_ntoa(*((struct in_addr *)host_entry->h_addr_list[0])));
}

void handle_client(int client_socket, const struct ProxyConfig *config) {
    struct sockaddr_in backend_addr;
    backend_addr.sin_family = AF_INET;
    backend_addr.sin_addr.s_addr = inet_addr(config->backend_ip);
    backend_addr.sin_port = htons(config->backend_port);

    int backend_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(backend_socket, (struct sockaddr *)&backend_addr, sizeof(backend_addr)) == -1) {
        perror("Unable to connect to the backend server");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    fd_set read_fds;
    int max_fd = (client_socket > backend_socket) ? client_socket : backend_socket;
    char buffer[BUFFER_SIZE];
    int bytes_received;

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(client_socket, &read_fds);
        FD_SET(backend_socket, &read_fds);

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("Error in select");
            close(client_socket);
            close(backend_socket);
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(client_socket, &read_fds)) {
            bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
            if (bytes_received <= 0) {
                break;
            }
            send(backend_socket, buffer, bytes_received, 0);
        }

        if (FD_ISSET(backend_socket, &read_fds)) {
            bytes_received = recv(backend_socket, buffer, sizeof(buffer), 0);
            if (bytes_received <= 0) {
                break;
            }
            send(client_socket, buffer, bytes_received, 0);
        }
    }

    close(client_socket);
    close(backend_socket);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <config_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    struct ProxyConfig config;
    load_config(argv[1], &config);

    char local_ip[INET_ADDRSTRLEN];
    get_local_ip(local_ip);

    int proxy_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (proxy_socket == -1) {
        perror("Unable to create socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in proxy_addr;
    proxy_addr.sin_family = AF_INET;
    proxy_addr.sin_addr.s_addr = INADDR_ANY;
    proxy_addr.sin_port = htons(config.frontend_port);

    if (bind(proxy_socket, (struct sockaddr *)&proxy_addr, sizeof(proxy_addr)) == -1) {
        perror("Unable to bind to port");
        close(proxy_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(proxy_socket, 10) == -1) {
        perror("Error while listening");
        close(proxy_socket);
        exit(EXIT_FAILURE);
    }

    printf("-- Listening for connections on %s:%d --\n", local_ip, config.frontend_port);
    printf("Proxying to %s:%d (%s)\n", config.backend_ip, config.backend_port, config.optional_note);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_socket = accept(proxy_socket, (struct sockaddr *)&client_addr, &client_addr_len);

        if (client_socket == -1) {
            perror("Error accepting connection");
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        printf("Received connection from %s\n", client_ip);

        if (fork() == 0) {
            close(proxy_socket);
            handle_client(client_socket, &config);
            printf("Connection closed from %s\n", client_ip);
            exit(EXIT_SUCCESS);
        }

        close(client_socket);
    }

    close(proxy_socket);

    return 0;
}
