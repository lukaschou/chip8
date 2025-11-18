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
    SDL_Window *window;
    SDL_Renderer *renderer;
} window;

typedef struct {
    // Program counter
    uint16_t PC;
    // Memory
    uint8_t memory[MEM_SIZE];
    // Registers
    uint8_t V[NUM_V_REGS];
    uint16_t I;
    // Timers
    uint8_t d_timer;
    uint8_t s_timer;
    // Stack
    int SP;
    uint16_t stack[STACK_SIZE];
    // Hardware states
    uint8_t display[DISPLAY_HEIGHT][DISPLAY_WIDTH];
    int keypad[16];
    int draw_flag;
} chip8_t;

int init_game(window *win);
void init_chip8(chip8_t *chip8);
int load_program(char *path, chip8_t *chip8);
void execute_cycle(chip8_t *chip8);

#endif
