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

unsigned char temp, i, unseeded;

// game stuff
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
unsigned char eel_direction[EEL_MAX_SIZE];

unsigned char eel_length;

#pragma bss-name(push, "XRAM")
// extra RAM at $6000-$7fff
unsigned char wram_array[0x2000];

#pragma bss-name(pop)

const unsigned char palette_bg[] = {
0x02,0x1a,0x2a,0x3a,0x02,0x07,0x17,0x3c,0x02,0x06,0x16,0x26,0x02,0x09,0x19,0x29
};

const unsigned char palette_spr[]={
                                   0x01,0x0f,0x10,0x30,
                                   0x01,0x16,0x2a,0x20,
                                   0x01,0x28,0x16,0x20,
                                   0x01,0x09,0x19,0x29
};

// the fixed bank

#pragma rodata-name ("RODATA")
#pragma code-name ("CODE")

void draw_sprites (void);
void start_game (void);

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

    switch(current_game_state) {
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
  current_game_state = Moving;

  ppu_off(); // screen off
  pal_bg(palette_bg); //	load the BG palette
  pal_spr(palette_spr); // load the sprite palette

  // draw some things
  vram_adr(NTADR_A(0,0));
  unrle(main_nametable);

  ppu_on_all();

  set_vram_buffer();
  clear_vram_buffer();
}

void draw_sprites (void) {
  oam_clear();
}
