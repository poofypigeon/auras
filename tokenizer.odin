package auras

import "core:unicode"

Tokenizer :: struct {
    line: string,
    token_start: uint,
    token_end: uint,
}

tokenizer_next :: proc(tokenizer: ^Tokenizer) -> (token: string, ok: bool) {    
    line := tokenizer.line

    // Skip whitespace
    for tokenizer.token_end != len(line) {
        if !unicode.is_space(rune(line[tokenizer.token_end])) {
            break
        }
        tokenizer.token_end += 1
    }
    tokenizer.token_start = tokenizer.token_end
    if tokenizer.token_end == len(line) {
        return "eol", false
    }

    // Treat comments as end-of-line
    if (line[tokenizer.token_start] == ';') {
        return "eol", false
    }

    // Any non-alphanumeric characters besides whitespace and underscores are distinct tokens
    tokenizer.token_end += 1;
    if !is_symbol_char(line[tokenizer.token_start]) {
        return line[tokenizer.token_start:tokenizer.token_end], true
    }

    // Consume characters until a non-label character is encountered
    for ; tokenizer.token_end != len(line); tokenizer.token_end += 1 {
        if !is_symbol_char(line[tokenizer.token_end]) { break }
    }
    return line[tokenizer.token_start:tokenizer.token_end], true
}

tokenizer_put_back :: #force_inline proc(tokenizer: ^Tokenizer) {    
    tokenizer.token_end = tokenizer.token_start
}

tokenizer_curr :: #force_inline proc(tokenizer: ^Tokenizer) -> string {
    return tokenizer.line[tokenizer.token_start:tokenizer.token_end]
}

is_symbol_char :: #force_inline proc(c: u8) -> bool {
    return unicode.is_alpha(rune(c)) || unicode.is_number(rune(c)) || c == '_'
}
