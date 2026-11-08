#include "webserver.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

static int load_file_with_fallback(const char *file_name, unsigned char **data, size_t *len) {
    const char *prefixes[] = {"", "..\\", "..\\..\\"};
    char full_path[512];
    FILE *file = NULL;
    long file_size;
    size_t read_size;
    int i;

    for (i = 0; i < 3; i++) {
        snprintf(full_path, sizeof(full_path), "%s%s", prefixes[i], file_name);
        file = fopen(full_path, "rb");
        if (file != NULL) {
            break;
        }
    }

    if (file == NULL) {
        return 0;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return 0;
    }

    file_size = ftell(file);
    if (file_size < 0) {
        fclose(file);
        return 0;
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return 0;
    }

    *data = (unsigned char *) malloc((size_t) file_size);
    if (*data == NULL) {
        fclose(file);
        return 0;
    }

    read_size = fread(*data, 1, (size_t) file_size, file);
    fclose(file);
    if (read_size != (size_t) file_size) {
        free(*data);
        *data = NULL;
        return 0;
    }

    *len = (size_t) file_size;
    return 1;
}

static void send_response(
    SOCKET client_socket,
    const char *status_line,
    const char *content_type,
    const unsigned char *body,
    size_t body_len
) {
    char header[512];
    int header_len = snprintf(
        header,
        sizeof(header),
        "%s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n",
        status_line,
        content_type,
        body_len
    );

    if (header_len > 0) {
        send(client_socket, header, header_len, 0);
        if (body_len > 0) {
            send(client_socket, (const char *) body, (int) body_len, 0);
        }
    }
}

int run_webserver(void) {
    WSADATA wsa_data;
    SOCKET server_socket = INVALID_SOCKET;
    struct sockaddr_in server_addr;
    const char *html =
        "<!doctype html>"
        "<html><head><meta charset=\"UTF-8\"><title>Chessboard</title>"
        "<style>"
        "body{font-family:Arial,sans-serif;display:flex;flex-direction:column;align-items:center;margin:24px;}"
        ".board{display:grid;grid-template-columns:repeat(8,60px);grid-template-rows:repeat(8,60px);border:2px solid #222;}"
        ".square{width:60px;height:60px;background-size:cover;}"
        ".white{background-image:url('/white_square.png');}"
        ".black{background-image:url('/black_square.png');}"
        "</style></head>"
        "<body><h1>Simple Chess Board</h1><div id=\"board\" class=\"board\"></div>"
        "<script>"
        "const board=document.getElementById('board');"
        "for(let r=0;r<8;r++){for(let c=0;c<8;c++){"
        "const s=document.createElement('div');"
        "s.className='square '+(((r+c)%2===0)?'white':'black');"
        "board.appendChild(s);"
        "}}"
        "</script></body></html>";
    const char *not_found = "404 Not Found";
    char request[2048];
    char path[256];

    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return 1;
    }

    server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET) {
        fprintf(stderr, "socket failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(8000);

    if (bind(server_socket, (const struct sockaddr *) &server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        fprintf(stderr, "bind failed: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
        fprintf(stderr, "listen failed: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    printf("Server running at http://localhost:8000\n");
    printf("Press Ctrl+C to stop.\n");

    while (1) {
        SOCKET client_socket = accept(server_socket, NULL, NULL);
        int received;
        if (client_socket == INVALID_SOCKET) {
            fprintf(stderr, "accept failed: %d\n", WSAGetLastError());
            continue;
        }

        received = recv(client_socket, request, sizeof(request) - 1, 0);
        if (received <= 0) {
            closesocket(client_socket);
            continue;
        }
        request[received] = '\0';
        path[0] = '\0';

        if (sscanf(request, "GET %255s", path) != 1) {
            snprintf(path, sizeof(path), "/");
        }

        if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) {
            send_response(
                client_socket,
                "HTTP/1.1 200 OK",
                "text/html; charset=UTF-8",
                (const unsigned char *) html,
                strlen(html)
            );
        } else if (strcmp(path, "/white_square.png") == 0 || strcmp(path, "/black_square.png") == 0) {
            unsigned char *image_data = NULL;
            size_t image_len = 0;
            const char *file_name = (path[1] == 'w') ? "white_square.png" : "black_square.png";

            if (load_file_with_fallback(file_name, &image_data, &image_len)) {
                send_response(client_socket, "HTTP/1.1 200 OK", "image/png", image_data, image_len);
                free(image_data);
            } else {
                send_response(
                    client_socket,
                    "HTTP/1.1 500 Internal Server Error",
                    "text/plain; charset=UTF-8",
                    (const unsigned char *) "PNG file missing",
                    strlen("PNG file missing")
                );
            }
        } else {
            send_response(
                client_socket,
                "HTTP/1.1 404 Not Found",
                "text/plain; charset=UTF-8",
                (const unsigned char *) not_found,
                strlen(not_found)
            );
        }

        closesocket(client_socket);
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}
