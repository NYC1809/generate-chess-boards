#include "webserver.h"
#include "puzzle_page.h"

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
        "body{font-family:Arial,sans-serif;display:flex;flex-direction:column;align-items:center;margin:24px;background:#f5f5f5;}"
        ".board{display:grid;grid-template-columns:repeat(8,64px);grid-template-rows:repeat(8,64px);border:2px solid #222;}"
        ".square{width:64px;height:64px;background-size:cover;position:relative;}"
        ".white{background-image:url('/textures/white_square.png');}"
        ".black{background-image:url('/textures/black_square.png');}"
        ".square.selected-move{outline:3px solid #0ea5e9;outline-offset:-3px;}"
        ".piece{width:56px;height:56px;position:absolute;left:4px;top:4px;user-select:none;pointer-events:none;}"
        ".palette{display:flex;flex-wrap:wrap;gap:8px;max-width:640px;justify-content:center;margin:0 0 12px 0;}"
        ".palette button{border:2px solid transparent;border-radius:8px;padding:4px 8px;background:#fff;cursor:pointer;}"
        ".palette button.active{border-color:#2563eb;background:#dbeafe;}"
        ".palette img{width:42px;height:42px;display:block;}"
        ".help{margin:8px 0 8px 0;color:#333;}"
        ".status{margin:0 0 16px 0;color:#111;font-size:14px;}"
        ".nav{margin:0 0 12px 0;}"
        ".nav a{display:inline-block;padding:8px 12px;border:1px solid #999;border-radius:8px;background:#fff;color:#111;text-decoration:none;}"
        "</style></head>"
        "<body><h1>Simple Chess Board</h1>"
        "<div class=\"nav\"><a href=\"/puzzle\">Open puzzle generator page</a></div>"
        "<div id=\"palette\" class=\"palette\"></div>"
        "<div class=\"help\">Palette mode: select a piece and click a square to place it. Move mode: click a board piece, then click destination. Right-click a square to clear it.</div>"
        "<div id=\"status\" class=\"status\"></div>"
        "<div id=\"board\" class=\"board\"></div>"
        "<script>"
        "const pieces=[['wK','/textures/white_king.png'],['wQ','/textures/white_queen.png'],['wR','/textures/white_rook.png'],['wB','/textures/white_bishop.png'],['wN','/textures/white_knight.png'],['wP','/textures/white_pawn.png'],['bK','/textures/black_king.png'],['bQ','/textures/black_queen.png'],['bR','/textures/black_rook.png'],['bB','/textures/black_bishop.png'],['bN','/textures/black_knight.png'],['bP','/textures/black_pawn.png']];"
        "const pieceMap=Object.fromEntries(pieces);"
        "const boardState=[['bR','bN','bB','bQ','bK','bB','bN','bR'],['bP','bP','bP','bP','bP','bP','bP','bP'],[null,null,null,null,null,null,null,null],[null,null,null,null,null,null,null,null],[null,null,null,null,null,null,null,null],[null,null,null,null,null,null,null,null],['wP','wP','wP','wP','wP','wP','wP','wP'],['wR','wN','wB','wQ','wK','wB','wN','wR']];"
        "let selectedPiece='wP';"
        "let selectedFrom=null;"
        "const palette=document.getElementById('palette');"
        "const board=document.getElementById('board');"
        "const status=document.getElementById('status');"
        "function getSquare(r,c){return board.children[r*8+c];}"
        "function clearMoveSelection(){if(selectedFrom){getSquare(selectedFrom.r,selectedFrom.c).classList.remove('selected-move');selectedFrom=null;}}"
        "function setMoveSelection(r,c){clearMoveSelection();selectedFrom={r:r,c:c};getSquare(r,c).classList.add('selected-move');}"
        "function updateStatus(){if(selectedPiece){status.textContent='Palette mode: '+selectedPiece+' selected.';}else if(selectedFrom){status.textContent='Move mode: selected '+boardState[selectedFrom.r][selectedFrom.c]+'. Click destination square.';}else{status.textContent='Move mode: click a piece to select it.';}}"
        "function renderPalette(){palette.innerHTML='';const clearBtn=document.createElement('button');clearBtn.textContent='Unselect';if(selectedPiece===null)clearBtn.className='active';clearBtn.onclick=()=>{selectedPiece=null;renderPalette();updateStatus();};palette.appendChild(clearBtn);pieces.forEach(([code,src])=>{const b=document.createElement('button');if(code===selectedPiece)b.className='active';b.title=code;b.innerHTML='<img alt=\"'+code+'\" src=\"'+src+'\">';b.onclick=()=>{selectedPiece=code;clearMoveSelection();renderPalette();updateStatus();};palette.appendChild(b);});}"
        "function paintSquare(square,pieceCode){square.innerHTML='';if(!pieceCode)return;const img=document.createElement('img');img.className='piece';img.alt=pieceCode;img.src=pieceMap[pieceCode];square.appendChild(img);}"
        "function paintBoard(){const squares=board.children;let i=0;for(let r=0;r<8;r++){for(let c=0;c<8;c++){paintSquare(squares[i++],boardState[r][c]);}}}"
        "for(let r=0;r<8;r++){for(let c=0;c<8;c++){const s=document.createElement('div');s.className='square '+(((r+c)%2===0)?'white':'black');s.dataset.r=r;s.dataset.c=c;s.addEventListener('click',()=>{if(selectedPiece){boardState[r][c]=selectedPiece;paintSquare(s,boardState[r][c]);clearMoveSelection();updateStatus();return;}if(selectedFrom){if(selectedFrom.r===r&&selectedFrom.c===c){clearMoveSelection();updateStatus();return;}const movingPiece=boardState[selectedFrom.r][selectedFrom.c];if(movingPiece){boardState[r][c]=movingPiece;boardState[selectedFrom.r][selectedFrom.c]=null;paintBoard();}clearMoveSelection();updateStatus();return;}if(boardState[r][c]){setMoveSelection(r,c);}updateStatus();});s.addEventListener('contextmenu',(e)=>{e.preventDefault();boardState[r][c]=null;paintSquare(s,null);if(selectedFrom&&selectedFrom.r===r&&selectedFrom.c===c){clearMoveSelection();}updateStatus();});board.appendChild(s);}}"
        "renderPalette();paintBoard();updateStatus();"
        "</script></body></html>";
    const char *not_found = "404 Not Found";
    char request[2048];
    char path[256];
    char route[256];

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
        route[0] = '\0';

        if (sscanf(request, "GET %255s", path) != 1) {
            snprintf(path, sizeof(path), "/");
        }
        snprintf(route, sizeof(route), "%s", path);
        {
            char *query_start = strchr(route, '?');
            if (query_start != NULL) {
                *query_start = '\0';
            }
        }

        if (strcmp(route, "/") == 0 || strcmp(route, "/index.html") == 0) {
            send_response(
                client_socket,
                "HTTP/1.1 200 OK",
                "text/html; charset=UTF-8",
                (const unsigned char *) html,
                strlen(html)
            );
        } else if (strcmp(route, "/puzzle") == 0) {
            const char *puzzle_html = get_puzzle_page_html();
            send_response(
                client_socket,
                "HTTP/1.1 200 OK",
                "text/html; charset=UTF-8",
                (const unsigned char *) puzzle_html,
                strlen(puzzle_html)
            );
        } else if (strncmp(route, "/textures/", 10) == 0 && strstr(route, "..") == NULL) {
            unsigned char *image_data = NULL;
            size_t image_len = 0;
            const char *file_name = route + 1;

            if (load_file_with_fallback(file_name, &image_data, &image_len)) {
                send_response(client_socket, "HTTP/1.1 200 OK", "image/png", image_data, image_len);
                free(image_data);
            } else {
                send_response(
                    client_socket,
                    "HTTP/1.1 404 Not Found",
                    "text/plain; charset=UTF-8",
                    (const unsigned char *) not_found,
                    strlen(not_found)
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
