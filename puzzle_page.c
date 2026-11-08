#include "puzzle_page.h"

#include "main.h"

#include <stdio.h>
#include <string.h>

static void build_board_array_js(const PuzzlePosition *position, char *output, size_t output_size) {
    size_t used = 0;
    int row;
    int col;

    used += snprintf(output + used, output_size - used, "[");
    for (row = 0; row < 8 && used < output_size; row++) {
        used += snprintf(output + used, output_size - used, "[");
        for (col = 0; col < 8 && used < output_size; col++) {
            used += snprintf(output + used, output_size - used, "'%s'%s", position->squares[row][col], col < 7 ? "," : "");
        }
        used += snprintf(output + used, output_size - used, "]%s", row < 7 ? "," : "");
    }
    snprintf(output + used, output_size - used, "]");
}

const char *get_puzzle_page_html(void) {
    static char html[32768];
    char board_array_js[2048];
    PuzzlePosition position = generate_puzzle_position();

    build_board_array_js(&position, board_array_js, sizeof(board_array_js));

    snprintf(
        html,
        sizeof(html),
        "<!doctype html>"
        "<html><head><meta charset=\"UTF-8\"><title>Chess Puzzle Generator</title>"
        "<style>"
        "body{font-family:Arial,sans-serif;display:flex;flex-direction:column;align-items:center;margin:24px;background:#f5f5f5;}"
        ".wrap{width:100%%;max-width:760px;display:flex;flex-direction:column;align-items:center;}"
        "h1{margin-bottom:8px;}"
        ".sub{color:#333;margin:0 0 16px 0;}"
        ".toolbar{display:flex;gap:8px;flex-wrap:wrap;justify-content:center;margin:0 0 16px 0;}"
        "input[type=text]{padding:10px 12px;font-size:15px;min-width:320px;border:1px solid #bbb;border-radius:8px;}"
        "button,a.btn{padding:10px 14px;font-size:14px;border:1px solid #999;border-radius:8px;background:#fff;color:#111;text-decoration:none;cursor:pointer;}"
        ".board{display:grid;grid-template-columns:repeat(8,64px);grid-template-rows:repeat(8,64px);border:2px solid #222;}"
        ".square{width:64px;height:64px;background-size:cover;position:relative;}"
        ".white{background-image:url('/textures/white_square.png');}"
        ".black{background-image:url('/textures/black_square.png');}"
        ".piece{width:56px;height:56px;position:absolute;left:4px;top:4px;pointer-events:none;}"
        ".array-view{margin-top:16px;width:100%%;max-width:640px;background:#fff;border:1px solid #bbb;border-radius:8px;padding:10px;font-size:13px;overflow:auto;}"
        "</style></head>"
        "<body><div class=\"wrap\">"
        "<h1>Chess Puzzle Generator</h1>"
        "<p class=\"sub\">Enter a seed and submit. Puzzle generation will be added later.</p>"
        "<form class=\"toolbar\" action=\"/puzzle\" method=\"get\">"
        "<input id=\"seedInput\" name=\"seed\" type=\"text\" placeholder=\"Enter seed (example: mate-in-2-42)\" required>"
        "<button type=\"submit\">Submit</button>"
        "<a class=\"btn\" href=\"/\">Back to board editor</a>"
        "</form>"
        "<div id=\"board\" class=\"board\"></div>"
        "<pre id=\"arrayView\" class=\"array-view\"></pre>"
        "</div>"
        "<script>"
        "const pieceMap={wK:'/textures/white_king.png',wQ:'/textures/white_queen.png',wR:'/textures/white_rook.png',wB:'/textures/white_bishop.png',wN:'/textures/white_knight.png',wP:'/textures/white_pawn.png',bK:'/textures/black_king.png',bQ:'/textures/black_queen.png',bR:'/textures/black_rook.png',bB:'/textures/black_bishop.png',bN:'/textures/black_knight.png',bP:'/textures/black_pawn.png'};"
        "const boardState=%s;"
        "const board=document.getElementById('board');"
        "const arrayView=document.getElementById('arrayView');"
        "arrayView.textContent='Generated 8x8 array:\\n'+JSON.stringify(boardState);"
        "for(let r=0;r<8;r++){for(let c=0;c<8;c++){const s=document.createElement('div');s.className='square '+(((r+c)%%2===0)?'white':'black');const code=boardState[r][c];if(code&&code!=='--'){const img=document.createElement('img');img.className='piece';img.alt=code;img.src=pieceMap[code];s.appendChild(img);}board.appendChild(s);}}"
        "</script></body></html>",
        board_array_js
    );

    return html;
}
