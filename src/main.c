/* Based on ...
 *  example of MMC3 for cc65
 *	Doug Fraker 2019
 */

#include "lib/neslib.h"
#include "lib/nesdoug.h"
#include "lib/unrle.h"
#include "mmc3/mmc3_code.h"
#include "mmc3/mmc3_code.c"
#include "sprites.h"
#include "../assets/nametables.h"

#pragma bss-name(push, "ZEROPAGE")

// GLOBAL VARIABLES

unsigned char arg1;
unsigned char arg2;
unsigned char pad1;
unsigned char pad1_new;
unsigned char double_buffer_index;

unsigned char temp, i, unseeded,
  temp_x, temp_y;

// game stuff

#define UP 0
#define DOWN 1
#define LEFT 2
#define RIGHT 3

#define EMPTY_TILE 0x00

enum game_state {
                 Title,
                 Moving,
                 Shocking,
                 GameOver
} current_game_state;

#pragma bss-name(pop)
// should be in the regular 0x300 ram now

unsigned char irq_array[32];
unsigned char double_buffer[32];

#define EEL_MAX_SIZE 64
unsigned char eel_x[EEL_MAX_SIZE];
unsigned char eel_y[EEL_MAX_SIZE];
unsigned char eel_dir[EEL_MAX_SIZE];

unsigned char eel_length, eel_head, eel_tail,
  eel_frame_counter, eel_speed, eel_growth;

unsigned char current_direction;

#pragma bss-name(push, "XRAM")
// extra RAM at $6000-$7fff
unsigned char wram_array[0x2000];

#pragma bss-name(pop)

// the fixed bank

#pragma rodata-name ("RODATA")
#pragma code-name ("CODE")

const unsigned char palette_bg[] = {
                                    0x02,0x1a,0x2a,0x3a,0x02,0x07,0x17,0x3c,0x02,0x06,0x16,0x26,0x02,0x09,0x19,0x29
};

const unsigned char palette_spr[]={
                                   0x01,0x0f,0x10,0x30,
                                   0x01,0x16,0x2a,0x20,
                                   0x01,0x28,0x16,0x20,
                                   0x01,0x09,0x19,0x29
};

const unsigned char head_per_direction[] = { 0x03, 0x04, 0x05, 0x06 };
const unsigned char tail_per_direction[] = { 0x0d, 0x0e, 0x0f, 0x10 };
const unsigned char middle_tile[] = {
                                     0x07, 0x07, 0x0a, 0x09,
                                     0x07, 0x07, 0x0b, 0x0c,
                                     0x0c, 0x09, 0x08, 0x08,
                                     0x0b, 0x0a, 0x08, 0x08
};

void draw_sprites (void);
void start_game (void);
void moving (void);

void main (void) {
  set_mirroring(MIRROR_HORIZONTAL);
  bank_spr(1);
  irq_array[0] = 0xff; // end of data
  set_irq_ptr(irq_array); // point to this array

  // clear the WRAM, not done by the init code
  // memfill(void *dst,unsigned char value,unsigned int len);
  memfill(wram_array,0,0x2000);

  ppu_off(); // screen off
  pal_bg(palette_bg); //	load the BG palette
  pal_spr(palette_spr); // load the sprite palette

  // draw some things
  vram_adr(NTADR_A(0,0));
  unrle(title_nametable);

  // load red alpha and drawing as bg chars
  // and unused as sprites
  set_chr_mode_2(0);
  set_chr_mode_3(1);
  set_chr_mode_4(2);
  set_chr_mode_5(3);
  set_chr_mode_0(4);
  set_chr_mode_1(6);

  music_play(0);

  draw_sprites();

  // game setup
  current_game_state = Title;

  unseeded = 1;


  ppu_on_all(); //	turn on screen

  set_vram_buffer();
  clear_vram_buffer();

  while (1){ // infinite loop
    ppu_wait_nmi();
    rand16();

    switch (current_game_state) {
    case Title:
      do {
        ppu_wait_nmi();
        pad_poll(0);
      } while(get_pad_new(0) == 0);
      if (unseeded) {
        seed_rng();
        unseeded = 0;
      }
      start_game();
      break;
    case Moving:
      moving();
    }

    // load the irq array with values it parse
    // ! CHANGED it, double buffered so we aren't editing the same
    // array that the irq system is reading from

    double_buffer_index = 0;

    // populate double buffer here

    double_buffer[double_buffer_index++] = 0xff; // end of data

    draw_sprites();

    // wait till the irq system is done before changing it
    // this could waste a lot of CPU time, so we do it last
    while(!is_irq_done() ){ // have we reached the 0xff, end of data
      // is_irq_done() returns zero if not done
      // do nothing while we wait
    }

    // copy from double_buffer to the irq_array
    // memcpy(void *dst,void *src,unsigned int len);
    memcpy(irq_array, double_buffer, sizeof(irq_array));
  }
}

void start_game (void) {
  ppu_off(); // screen off
  pal_bg(palette_bg); //	load the BG palette
  pal_spr(palette_spr); // load the sprite palette

  // draw some things
  vram_adr(NTADR_A(0,0));
  unrle(main_nametable);

  ppu_on_all();

  current_game_state = Moving;

  eel_length = 5;
  eel_x[0] = 7;
  eel_x[1] = 8;
  eel_x[2] = 9;
  eel_x[3] = 10;
  eel_x[4] = 11;
  eel_y[0] = eel_y[1] = eel_y[2] = eel_y[3] = eel_y[4] = 15;
  eel_dir[0] = eel_dir[1] = eel_dir[2] = eel_dir[3] = eel_dir[4] = RIGHT;
  eel_head = 4;
  eel_tail = 0;
  eel_speed = 2;
  eel_growth = 0;
  eel_frame_counter = 0;
  current_direction = RIGHT;
}

void erase_tail (unsigned char x, unsigned char y) {
  one_vram_buffer(EMPTY_TILE, NTADR_A(x, y));
}

void draw_tail (unsigned char x, unsigned char y, unsigned char direction) {
  one_vram_buffer(tail_per_direction[direction], NTADR_A(x, y));
}

void erase_head (unsigned char x, unsigned char y, unsigned char old_direction, unsigned char direction) {
  one_vram_buffer(middle_tile[(old_direction << 2) | direction], NTADR_A(x, y));
}

void draw_head (unsigned char x, unsigned char y, unsigned char direction) {
  one_vram_buffer(head_per_direction[direction], NTADR_A(x, y));
}

void move_eel (void) {
  eel_frame_counter += eel_speed;
  if (eel_frame_counter < 60) return;
  eel_frame_counter -= 60;

  temp_x = eel_x[eel_head];
  temp_y = eel_y[eel_head];

  switch (current_direction) {
  case UP:
    --temp_y;
    break;
  case DOWN:
    ++temp_y;
    break;
  case LEFT:
    --temp_x;
    break;
  case RIGHT:
    ++temp_x;
    break;
  }

  if (eel_growth > 0) {
    --eel_growth;
    ++eel_length;
  } else {
    erase_tail(eel_x[eel_tail], eel_y[eel_tail]);
    eel_tail = (eel_tail + 1) % EEL_MAX_SIZE;
    draw_tail(eel_x[eel_tail], eel_y[eel_tail], eel_dir[(eel_tail + 1) % EEL_MAX_SIZE]);
  }

  erase_head(eel_x[eel_head], eel_y[eel_head], eel_dir[eel_head], current_direction);
  eel_head = (eel_head + 1) % EEL_MAX_SIZE;
  eel_x[eel_head] = temp_x;
  eel_y[eel_head] = temp_y;
  eel_dir[eel_head] = current_direction;
  draw_head(temp_x, temp_y, current_direction);
}

void handle_moving_input (void) {
  pad_poll(0);
  temp = get_pad_new(0);
  // TODO: buffer
  if (temp & PAD_UP) {
    current_direction = UP;
  } else if (temp & PAD_DOWN) {
    current_direction = DOWN;
  } else if (temp & PAD_LEFT) {
    current_direction = LEFT;
  } else if (temp & PAD_RIGHT) {
    current_direction = RIGHT;
  }
}

void moving (void) {
  clear_vram_buffer();
  handle_moving_input();
  move_eel();
}

void draw_sprites (void) {
  oam_clear();
}
