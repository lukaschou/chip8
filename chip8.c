#include "chip8.h"
#include <SDL3/SDL_main.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

int init(chip8_t *chip8, SDL_Window *window, SDL_Renderer *renderer) {
    if (!SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO)) {
        fprintf(stderr, "Failed to initialize SDL: %s", SDL_GetError());
        return -1;
    }
    
    window = SDL_CreateWindow(
        "Chip8",
        640,
        320,
        0
    );

    if (window == NULL) {
        fprintf(stderr, "Failed to initialize SDL: %s", SDL_GetError());
        return -1;
    }

    renderer = SDL_CreateRenderer(window, NULL);
    if (renderer == NULL) {
        fprintf(stderr, "Failed to init SDL renderer: %s", SDL_GetError());
        return -1;
    }

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
    return 0;
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

void handle_instruction(uint16_t instruction) {
    uint16_t f = (instruction & 0xF000) >> 12;
    uint16_t nnn = instruction & 0x0FFF;
    uint16_t nn = instruction & 0x00FF;
    uint16_t n = instruction & 0x000F;
    uint16_t x = instruction & 0x0F00;
    uint16_t y = instruction & 0x00F0;

    printf("Instruction: %x\n", instruction);
    printf("First nibble: %x\n", f);
    printf("NNN: %x\n", nnn);
    printf("NN: %x\n", nn);
    printf("N: %x\n", n);
    printf("X: %x\n", x);
    printf("Y: %x\n", y);

    switch (f) {
        case 0x0:
            if (nnn == 0x0E0) {
                printf("Clear screen\n");
            }
            break;
        case 0x1:
            printf("jump to %x\n", nnn);
            break;
        case 0x6:
            printf("set register %x to %x\n", x, nn);
    }
    
    return;
}

void cleanup() {
    SDL_Quit();
}


int main(int argc, char *argv[]) {
    chip8_t chip8;
    SDL_Window *window;
    SDL_Renderer *renderer;
    
    if (argc != 2) {
        fprintf(stderr, "usage: chip8 [program]");
        return -1;
    }

    printf("Initializing emulator...\n");
    if (init(&chip8, window, renderer) == -1) {
        return -1;
    }
    printf("Loading program...\n");
    if (load_program(argv[1], &chip8) == -1) {
        fprintf(stderr, "Failed to load program %s", argv[1]);
        return -1;
    }
    printf("Loaded program %s\n", argv[1]);
    
    int i = 0;
    while (i < 20) {
        // Fetch instruction
        uint16_t instruction = (chip8.memory[chip8.PC] << 8) | chip8.memory[chip8.PC + 1];
        chip8.PC += 2;
        // Decode instruction
        handle_instruction(instruction);
        i++;
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
        SDL_RenderPresent(renderer);
    }
    return 0;
}
