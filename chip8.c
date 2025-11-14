#include "chip8.h"
#include "SDL3/SDL_scancode.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

const static SDL_Scancode keypad[16] = {
    SDL_SCANCODE_X, SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3,
    SDL_SCANCODE_Q, SDL_SCANCODE_W, SDL_SCANCODE_E, SDL_SCANCODE_A,
    SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_Z, SDL_SCANCODE_C,
    SDL_SCANCODE_4, SDL_SCANCODE_R, SDL_SCANCODE_F, SDL_SCANCODE_V
};

const static uint8_t fontset[FONTSET_SIZE] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

static void bye() {
    SDL_Quit();
}

int init_game(window *win) {
    // Seed random num generator for CXNN opcode
    srand(time(NULL));

    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) == false) {
        return -1;
    }

    window = SDL_CreateWindow(
            "Chip8",
            PIXEL_SIZE * DISPLAY_WIDTH,
            PIXEL_SIZE * DISPLAY_HEIGHT,
            0
    );
    if (window == NULL) {
        return -1;
    }

    renderer = SDL_CreateRenderer(window, NULL);
    if (renderer == NULL) {
        SDL_DestroyWindow(window);
        return -1;
    }
    
    win->window = window;
    win->renderer = renderer;
    return 0;
}

void init_chip8(chip8_t *chip8) {
    // Set PC to prog start
    chip8->PC = PROG_START_ADDR;
    
    // Clear memory
    for (int i=0; i < MEM_SIZE; i++) {
        chip8->memory[i] = 0;
    }
    // Load font
    for (int i=0; i < FONTSET_SIZE; i++) {
        chip8->memory[i] = fontset[i];
    }

    // Clear Registers
    for (int i=0; i <= NUM_V_REGS; i++) {
        chip8->V[i] = 0;
    }
    chip8->I = 0;
    
    // Reset keypad
    for (int i=0; i <= 0xF; i++) {
        chip8->keypad[i] = 0;
    }

    // Clear stack
    chip8->SP = 0;

    // Reset timers
    chip8->d_timer = 0;
} 

int load_program(char *path, chip8_t *chip8) {
    FILE *source = fopen(path, "rb");
    if (source == NULL) {
        perror("fopen");
        return -1;
    }
    
    uint8_t buffer[MAX_PROG_LEN];
    size_t len = fread(buffer, sizeof(uint8_t), MAX_PROG_LEN, source);
    if (ferror(source) != 0) {
        perror("fread");
        fclose(source);
        return -1;
    }
    fclose(source);
    
    for (int i = 0; i < len; i++) {
        chip8->memory[i+PROG_START_ADDR] = buffer[i];
    }
    return 0;
}

void draw(chip8_t *chip8, SDL_Renderer *renderer) {
    for (int row=0; row < DISPLAY_HEIGHT; row++) {
        for (int col=0; col < DISPLAY_WIDTH; col++) {
            if (chip8->display[row][col]) {
                SDL_FRect pixel;
                pixel.x = col * PIXEL_SIZE;
                pixel.y = row * PIXEL_SIZE;
                pixel.w = PIXEL_SIZE;
                pixel.h = PIXEL_SIZE;
                if (SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255) == false) {
                    fprintf(stderr, "Warning: Failed to set pixel draw color: %s\n", SDL_GetError());
                }

                if (SDL_RenderFillRect(renderer, &pixel) == false) {
                    fprintf(stderr, "Warning: Failed to draw pixel: %s\n", SDL_GetError());
                }
            }
        }
    }
}



void handle_instruction(uint16_t instruction, chip8_t *chip8) {
    uint16_t first_nibble = (instruction & 0xF000) >> 12;
    uint16_t nnn = instruction & 0x0FFF;
    uint8_t nn = instruction & 0x00FF;
    uint8_t n = instruction & 0x000F;
    uint16_t x = (instruction & 0x0F00) >> 8;
    uint16_t y = (instruction & 0x00F0) >> 4;

    switch (first_nibble) {
        case 0x0:
            switch (nnn) {
                case 0x0E0:
                    // Clear screen
                    for (int y=0; y < DISPLAY_HEIGHT; y++) {
                        for (int x=0; x < DISPLAY_WIDTH; x++) {
                            chip8->display[y][x] = 0; 
                        }
                    }
                    break;
                case 0x0EE:
                    // Return from subroutine
                    chip8->PC = chip8->stack[chip8->SP];
                    chip8->SP--;
                    break;
            }

            break;
        case 0x1:
            // Jump
            chip8->PC = nnn;
            break;
        case 0x2:
            // Call subroutine
            chip8->SP++;
            chip8->stack[chip8->SP] = chip8->PC;
            chip8->PC = nnn;
            break;
        case 0x3:
            // Skip instruction if Vx == nn
            if (chip8->V[x] == nn) {
                chip8->PC += 2;
            }
            break;
        case 0x4:
            // Skip instruction if Vx != nn
            if (chip8->V[x] != nn) {
                chip8->PC += 2;
            }
            break;
        case 0x5:
            // Skip instruction if Vx == Vy
            if (chip8->V[x] == chip8->V[y]) {
                chip8->PC += 2;
            }
            break;
        case 0x6:
            chip8->V[x] = nn;
            break;
        case 0x7:
            chip8->V[x] += nn;
            break;
        case 0x8:
            // Math / bitops
            switch (n) {
                case 0x0:
                    chip8->V[x] = chip8->V[y];
                    break;
                case 0x1:
                    chip8->V[x] |= chip8->V[y];
                    break;
                case 0x2:
                    chip8->V[x] &= chip8->V[y];
                    break;
                case 0x3:
                    chip8->V[x] ^= chip8->V[y];
                    break;
                case 0x4: 
                    {   // Add Vx to Vy and sets VF to 1 on oveflow
                        int sum = chip8->V[x] + chip8->V[y];
                        chip8->V[x] = sum;
                        if (sum > 255) {
                            chip8->V[0xF] = 1;
                        } else {
                            chip8->V[0xF] = 0;
                        }
                    }
                    break;
                case 0x5:
                    {   // Subtract Vy from Vx and set VF to 0 on underflow
                        int diff = chip8->V[x] - chip8->V[y];
                        chip8->V[x] = diff;
                        if (diff >= 0) {
                            chip8->V[0xF] = 1;
                        } else {
                            chip8->V[0xF] = 0;
                        }
                    } 
                    break;
                case 0x6:
                    {   // Shift Vx to the right and store old LSB in VF
                        uint8_t bit = 0x01 & chip8->V[x];
                        chip8->V[x] >>= 1;
                        chip8->V[0xF] = bit;
                    }
                    break;
                case 0x7:
                    {   // Set Vx to Vy - Vx and set VF to 0 on underflow
                        int diff = chip8->V[y] - chip8->V[x];
                        chip8->V[x] = diff;
                        if (diff >= 0) {
                            chip8->V[0xF] = 1;
                        } else {
                            chip8->V[0xF] = 0;
                        }
                    }
                    break;
                case 0xE:
                    {   // Shift Vx to the left and store old MSB in VF
                        uint8_t bit = (0x80 & chip8->V[x]) >> 7;
                        chip8->V[x] <<= 1;
                        chip8->V[0xF] = bit;
                    }
                    break;
            }
            break;
        case 0x9:
            // Skip instruction if Vx != Vy
            if (chip8->V[x] != chip8->V[y]) {
                chip8->PC += 2;
            }
            break;
        case 0xA:
            // Set index register I
            chip8->I = nnn;
            break;
        case 0xB:
            // Jump to address V0 + nnn
            chip8->PC = chip8->V[0] + nnn;
            break;
        case 0xC:
            // Set Vx to bitwise AND of nn and a random number
            chip8->V[x] = (rand() % 256) & nn;
            break;
        case 0xD:
            {   // Draw
                uint8_t x_coord = chip8->V[x] % DISPLAY_WIDTH;
                uint8_t y_coord = chip8->V[y] % DISPLAY_HEIGHT;
                chip8->V[0xF] = 0;
                for (uint8_t row = 0; row < n; row++) {
                    // Stop if we've reached the bottom of the screen
                    if (y_coord + row >= DISPLAY_HEIGHT) {
                        break;
                    }

                    uint8_t sprite_byte = chip8->memory[chip8->I + row];
                    for (int bit_index = 0; bit_index < 8; bit_index++) {
                        // Stop if we've reached the edge of the screen
                        if (x_coord + bit_index >= DISPLAY_WIDTH) {
                            break;
                        } 
                        
                        uint8_t bit = sprite_byte & (1 << (7 - bit_index));
                        // Set VF if a pixel is turned off
                        if (bit & chip8->display[y_coord + row][x_coord + bit_index]) {
                            chip8->V[0xF] = 1;
                        }
                        chip8->display[y_coord + row][x_coord + bit_index] ^= bit;
                    }  
                }   
            } 
            break;
        case 0xE:
            switch (nn) {
                case 0x9E:
                    // Skip instruction if key in Vx is pressed
                    if (chip8->keypad[chip8->V[x] & 0x01]) {
                        chip8->PC += 2;
                    }
                    break;
                case 0xA1:
                    // Skip instruction if key in Vx is not pressed
                    if (!(chip8->keypad[chip8->V[x] & 0x01])) {
                        chip8->PC += 2;
                    }
                    break;
            }
            break;
        case 0xF:
            switch(nn) {
                case 0x07: 
                    chip8->V[x] = chip8->d_timer; 
                    break; 
                case 0x0A:
                    {   // Await keypress, then store in Vx
                        chip8->PC -= 2;
                        for (int i = 0; i <= 0xF; i++) {
                            if (chip8->keypad[i]) {
                                chip8->V[x] = i;
                                chip8->PC += 4;
                                break;   
                            }
                        }
                    } 
                    break;
                case 0x15: 
                    chip8->d_timer = chip8->V[x];
                    break; 
                case 0x18:
                    chip8->s_timer = chip8->V[x];
                case 0x1E: 
                    chip8->I += chip8->V[x];
                    break;
                case 0x29:
                    // Set I to the location of the sprite for the char in Vx
                    chip8->I = (chip8->V[x] & 0x01) * 5;
                    break;
                case 0x33:
                    // Stores BCD representation of Vx in I
                    chip8->memory[chip8->I] = chip8->V[x] / 100;
                    chip8->memory[chip8->I + 1] = (chip8->V[x] % 100) / 10;
                    chip8->memory[chip8->I + 2] = chip8->V[x] % 10;
                    break;
                case 0x55:
                    for (int i = 0; i <= x; i++) {
                        chip8->memory[chip8->I + i] = chip8->V[i];
                    }
                    break;
                case 0x65:
                    for (int i = 0; i <= x; i++) {
                        chip8->V[i] = chip8->memory[chip8->I + i];
                    }
                    break;
            }
            break;
        }
    
    return;
}

int main(int argc, char *argv[]) {
    chip8_t chip8;
    window win;
    
    if (argc != 2) {
        fprintf(stderr, "usage: chip8 [program]");
        exit(EXIT_FAILURE);
    }
    
    if (init_game(&win) == -1) {
        fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    init_chip8(&chip8);
    
    if (load_program(argv[1], &chip8) == -1) {
        fprintf(stderr, "Failed to load program %s\n", argv[1]);
        exit(EXIT_FAILURE);
    } 
    chip8.memory[0x1FF] = 1;
    int done = 0;
    while (!done) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                done = 1;
            }

            if (event.type == SDL_EVENT_KEY_DOWN) {
                for (int i = 0; i <= 0xF; i++) {
                    if (keypad[i] == event.key.scancode) {
                        chip8.keypad[i] = 1;
                        printf("Key: %d\n", i);
                        break;
                    }
                } 
            }

            if (event.type == SDL_EVENT_KEY_UP) {
                for (int i = 0; i <= 0xF; i++) {
                    if (keypad[i] == event.key.scancode) {
                        chip8.keypad[i] = 0;
                        break;
                    }
                }
            }
        }

        SDL_SetRenderDrawColor(win.renderer, 0, 0, 0, 255);
        SDL_RenderClear(win.renderer);
        // Fetch instruction
        uint16_t instruction = (chip8.memory[chip8.PC] << 8) | chip8.memory[chip8.PC + 1];
        chip8.PC += 2;
        // Decode instruction
        handle_instruction(instruction, &chip8);
        draw(&chip8, win.renderer);
        SDL_RenderPresent(win.renderer);
    }
    return 0;
}
