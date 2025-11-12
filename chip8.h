#ifndef CHIP8_H
#define CHIP8_H

#include <SDL3/SDL.h>
#include <stdint.h>

#define MEM_SIZE 4096
#define NUM_V_REGS 16
#define FONTSET_SIZE 80 
#define PROG_START_ADDR 0x200
#define MAX_PROG_LEN (MEM_SIZE - PROG_START_ADDR)
#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32
#define PIXEL_SIZE 8
#define STACK_SIZE 100

typedef struct {
    int top;
    uint16_t stack[STACK_SIZE];
} ch8stack_t;

typedef struct {
    uint8_t memory[MEM_SIZE];
    uint8_t V[NUM_V_REGS];
    uint16_t I;
    uint16_t PC;
    uint8_t display[DISPLAY_HEIGHT][DISPLAY_WIDTH];
    ch8stack_t stack;
    int keypad[16];
} chip8_t;

int init_sdl(SDL_Window **window, SDL_Renderer **renderer);
void init_chip8(chip8_t *chip8);
int load_program(char *path, chip8_t *chip8);
void handle_instruction(uint16_t instruction, chip8_t *chip8);
int is_valid_scancode(SDL_Scancode sc);

#endif
