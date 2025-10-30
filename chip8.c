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

int init_sdl(SDL_Window *win, SDL_Renderer *ren) {
    if (!SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO)) {
        return -1;
    }

    win = SDL_CreateWindow("Chip8", 640, 320, 0);
    if (win == NULL) {
        return -1;
    }

    ren = SDL_CreateRenderer(win, NULL);
    if (ren == NULL) {
        return -1;
    }

    return 0;
}

void init_chip8(chip8_t *chip8) {
    chip8->PC = PROG_START_ADDR; // PC starts at x200
    // Clear memory
    for (int i=0; i < MEM_SIZE; i++) {
        chip8->memory[i] = 0;
    }
    // Clear Registers
    for (int i=0; i < NUM_V_REGS; i++) {
        chip8->V[i] = 0;
    }
    chip8->I = 0;
} 

int load_program(char *path, chip8_t *chip8) {
    FILE *source = fopen(path, "rb");
    if (source == NULL) {
        perror("fopen");
        return -1;
    }
    
    int maxBufLen = MEM_SIZE - 0x200 + 1;
    uint8_t buffer[maxBufLen];
    size_t len = fread(buffer, sizeof(u_int8_t), maxBufLen, source);
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

void print_display(chip8_t *chip8) {
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 64; x++) {
            putchar(chip8->display[y][x] ? '*' : ' ');
        }
        putchar('\n');
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
            if (nnn == 0x0E0) {
                // Clear screen
                printf("Clearing screen...\n");
                for (int y=0; y < 32; y++) {
                    for (int x=0; x < 64; x++) {
                        chip8->display[y][x] = 0; 
                    }
                }
            }
            break;
        case 0x1:
            // Jump
            // printf("Jumping to address %x...\n", nnn);
            chip8->PC = nnn;
            break;
        case 0x6:
            // Set register VX
            printf("Setting register %x to %x...\n", x, nn);
            chip8->V[x] = nn;
            break;
        case 0x7:
            // Add value to register VX
            printf("Adding %x to regist %x...\n", x, nn);
            chip8->V[x] += nn;
            break;
        case 0xA:
            // Set index register I
            printf("Setting index register I to %x...\n", nnn);
            chip8->I = nnn;
            break;
        case 0xD:
            // Display/draw
            uint8_t horizontal = chip8->V[x & 63];
            uint8_t vertical = chip8->V[y & 31];
            chip8->V[0xF] = 0;

            printf("Drawing at position %x, %x...\n", horizontal, vertical);
            for (uint8_t i = 0; i < n; i++) {
                if (vertical + i > 31) {
                    printf("Pixel is below the screen, clipping...\n");
                    break;
                }

                uint8_t sprite = chip8->memory[chip8->I+i];
                printf("Sprite %x: %x\n", i, sprite); 
                int bit_index = 0;
                while (bit_index < 8) {
                    if (horizontal + bit_index > 63) {
                        printf("pixel is outside of screen, clipping...\n");
                        break;
                    } 

                    uint8_t bit = (sprite >> (7 - bit_index)) & 1;
                    printf("Bit %x is %x\n", bit_index, bit);
                    if (bit & chip8->display[vertical+i][horizontal+bit_index]) {
                        chip8->V[0xF] = 1;
                    }
                    chip8->display[vertical+i][horizontal+bit_index] ^= bit;
                    bit_index++;
                } 
            }
            print_display(chip8);
        }
    
    return;
}

int main(int argc, char *argv[]) {
    chip8_t chip8;
    SDL_Window *window;
    SDL_Renderer *renderer;
    
    if (argc != 2) {
        fprintf(stderr, "usage: chip8 [program]");
        return -1;
    }

    if (init_sdl(window, renderer) == -1) {
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

        // Fetch instruction
        uint16_t instruction = (chip8.memory[chip8.PC] << 8) | chip8.memory[chip8.PC + 1];
        chip8.PC += 2;
        // Decode instruction
        handle_instruction(instruction, &chip8);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
    }
    return 0;
}
