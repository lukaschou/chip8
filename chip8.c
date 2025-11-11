#include "chip8.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

static void bye() {
    SDL_Quit();
}

int init_sdl(SDL_Window **win, SDL_Renderer **ren) {
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
    
    *win = window;
    *ren = renderer;
    return 0;
}

void init_chip8(chip8_t *chip8) {
    // Set PC to prog start
    chip8->PC = PROG_START_ADDR;
    // Clear memory
    for (int i=0; i < MEM_SIZE; i++) {
        chip8->memory[i] = 0;
    }
    // Clear Registers
    for (int i=0; i < NUM_V_REGS; i++) {
        chip8->V[i] = 0;
    }
    chip8->I = 0;
    // Make sure stack pointer is set to 0
    chip8->stack.top = 0;
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
    uint16_t f = (instruction & 0xF000) >> 12;
    uint16_t nnn = instruction & 0x0FFF;
    uint8_t nn = instruction & 0x00FF;
    uint8_t n = instruction & 0x000F;
    uint16_t x = (instruction & 0x0F00) >> 8;
    uint16_t y = (instruction & 0x00F0) >> 4;

    switch (f) {
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
                    chip8->PC = chip8->stack.stack[chip8->stack.top];
                    chip8->stack.top--;
                    break;
            }

            break;
        case 0x1:
            // Jump
            chip8->PC = nnn;
            break;
        case 0x2:
            // Call subroutine
            chip8->stack.top++;
            chip8->stack.stack[chip8->stack.top] = chip8->PC;
            chip8->PC = nnn;
            break;
        case 0x3:
            if (chip8->V[x] == nn) {
                chip8->PC += 2;
            }
            break;
        case 0x4:
            if (chip8->V[x] != nn) {
                chip8->PC += 2;
            }
            break;
        case 0x5:
            if (chip8->V[x] == chip8->V[y]) {
                chip8->PC += 2;
            }
            break;
        case 0x6:
            // Set register VX
            chip8->V[x] = nn;
            break;
        case 0x7:
            // Add value to register VX
            chip8->V[x] += nn;
            break;
        case 0x8:
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
                    {
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
                    {
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
                    {
                        uint8_t bit = 0x01 & chip8->V[x];
                        chip8->V[x] >>= 1;
                        chip8->V[0xF] = bit;
                    }
                    break;
                case 0x7:
                    {
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
                    {
                        uint8_t bit = (0x80 & chip8->V[x]) >> 7;
                        chip8->V[x] <<= 1;
                        chip8->V[0xF] = bit;
                    }
                    break;
            }
            break;
        case 0x9:
            if (chip8->V[x] != chip8->V[y]) {
                chip8->PC += 2;
            }
            break;
        case 0xA:
            // Set index register I
            chip8->I = nnn;
            break;
        case 0xB:
            chip8->PC = chip8->V[0] + nnn;
            break;
        case 0xC:
            chip8->V[x] = (rand() % 256) & nn;
            break;
        case 0xD:
            // Display/draw
            {}
            uint8_t horizontal = chip8->V[x & 63];
            uint8_t vertical = chip8->V[y & 31];
            chip8->V[0xF] = 0;
            for (uint8_t i = 0; i < n; i++) {
                if (vertical + i > 31) {
                    break;
                }

                uint8_t sprite = chip8->memory[chip8->I+i];
                int bit_index = 0;
                while (bit_index < 8) {
                    if (horizontal + bit_index > 63) {
                        break;
                    } 

                    uint8_t bit = (sprite >> (7 - bit_index)) & 1;
                    if (bit & chip8->display[vertical+i][horizontal+bit_index]) {
                        chip8->V[0xF] = 1;
                    }
                    chip8->display[vertical+i][horizontal+bit_index] ^= bit;
                    bit_index++;
                } 
            }
            break;
        case 0xF:
            switch(nn) {
                case 0x1E:
                    chip8->I += chip8->V[x];
                    break;
                case 0x33:
                    chip8->memory[chip8->I] = chip8->V[x] / 100;
                    chip8->memory[chip8->I + 1] = (chip8->V[x] % 100) / 10;
                    chip8->memory[chip8->I + 2] = chip8->V[x] % 10;
                    break;
                case 0x55:
                    for (int i=0; i <= x; i++) {
                        chip8->memory[chip8->I + i] = chip8->V[i];
                    }
                    break;
                case 0x65:
                    for (int i=0; i <= x; i++) {
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
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    
    if (argc != 2) {
        fprintf(stderr, "usage: chip8 [program]");
        return -1;
    }
    
    srand(time(NULL));

    if (init_sdl(&window, &renderer) == -1) {
        fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    init_chip8(&chip8);
    
    if (load_program(argv[1], &chip8) == -1) {
        fprintf(stderr, "Failed to load program %s\n", argv[1]);
        exit(EXIT_FAILURE);
    } 
        
    int done = 0;
    while (!done) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                done = true;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        // Fetch instruction
        uint16_t instruction = (chip8.memory[chip8.PC] << 8) | chip8.memory[chip8.PC + 1];
        chip8.PC += 2;
        // Decode instruction
        handle_instruction(instruction, &chip8);
        draw(&chip8, renderer);
        SDL_RenderPresent(renderer);
    }
    return 0;
}
