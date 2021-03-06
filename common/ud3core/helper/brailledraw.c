/*
 * UD3
 *
 * Copyright (c) 2018 Jens Kerrinnes
 * Copyright (c) 2015 Steve Ward
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "brailledraw.h"
#include "cli_common.h"
#include <stdlib.h>

const unsigned char pixmap[4][2] = {
	{0x01, 0x08},
	{0x02, 0x10},
	{0x04, 0x20},
	{0x40, 0x80}};

typedef struct pix_struct pix_str;
struct pix_struct {
    uint8_t data[PIX_HEIGHT];
};

pix_str *pix_buffer[PIX_WIDTH / 8];

char out_buffer[3] = {0x00, 0x00, 0x00};

void raise_mermory_error(port_str *ptr){
    SEND_CONST_STRING("WARNING: NULL pointer malloc failed\r\n", ptr);
}

static void set_bytes(char *pbuffer, const unsigned char c) {
	pbuffer[0] = (char)0xE2;
	if (c & pixmap[3][0] && c & pixmap[3][1]) {
		pbuffer[1] = (char)0xA3;
	} else if (c & pixmap[3][1]) {
		pbuffer[1] = (char)0xA2;
	} else if (c & pixmap[3][0]) {
		pbuffer[1] = (char)0xA1;
	} else {
		pbuffer[1] = (char)0xA0;
	}

	pbuffer[2] = (char)((0xBF & c) | 0x80);
}

void braille_malloc(port_str *ptr){
    for(uint8_t i=0;i<PIX_WIDTH / 8;i++){
        pix_buffer[i] =  pvPortMalloc(sizeof(pix_str));
        if(pix_buffer[i]==NULL)
            raise_mermory_error(ptr);
    }
}
void braille_free(port_str *ptr){
   for(uint8_t i=0;i<PIX_WIDTH / 8;i++){
        if(pix_buffer[i]==NULL){
            raise_mermory_error(ptr);
        }else{
            vPortFree(pix_buffer[i]);
        }
    }
}

void braille_setPixel(uint8_t x, uint8_t y) {
	if (x > PIX_WIDTH - 1)
		x = PIX_WIDTH - 1;
	if (y > PIX_HEIGHT - 1)
		y = PIX_HEIGHT - 1;
	uint8_t byte = x / 8;
	uint8_t bit = x % 8;
    if(pix_buffer[byte]==NULL) return;
    pix_buffer[byte]->data[y] |= 1 << bit;
}

void braille_line(int x0, int y0, int x1, int y1) {

	int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
	int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
	int err = (dx > dy ? dx : -dy) / 2, e2;

	for (;;) {
		braille_setPixel(x0, y0);
		if (x0 == x1 && y0 == y1)
			break;
		e2 = err;
		if (e2 > -dx) {
			err -= dy;
			x0 += sx;
		}
		if (e2 < dy) {
			err += dx;
			y0 += sy;
		}
	}
}

void braille_clear(void) {
	for (uint8_t x = 0; x < PIX_WIDTH / 8; x++) {
		for (uint8_t y = 0; y < PIX_HEIGHT; y++) {
            if(pix_buffer[x]!=NULL)
                pix_buffer[x]->data[y] = 0x00;
		}
	}
}

void braille_draw(port_str *ptr) {
	unsigned char byte;
	for (uint16_t y = 0; y < PIX_HEIGHT; y += 4) {
		for (uint16_t x = 0; x < PIX_WIDTH / 8; x++) {
			for (uint16_t bpos = 0; bpos < 8; bpos += 2) {
				byte = 0x00;
                if(pix_buffer[x]==NULL){
                    raise_mermory_error(ptr);
                    return;
                }
				if (pix_buffer[x]->data[y] & (1 << bpos))
					byte |= 1 << 0;
				if (pix_buffer[x]->data[y] & (1 << (bpos + 1)))
					byte |= 1 << 3;

				if (pix_buffer[x]->data[y+1] & (1 << bpos))
					byte |= 1 << 1;
				if (pix_buffer[x]->data[y+1] & (1 << (bpos + 1)))
					byte |= 1 << 4;

				if (pix_buffer[x]->data[y+2] & (1 << bpos))
					byte |= 1 << 2;
				if (pix_buffer[x]->data[y+2] & (1 << (bpos + 1)))
					byte |= 1 << 5;

				if (pix_buffer[x]->data[y+3] & (1 << bpos))
					byte |= 1 << 6;
				if (pix_buffer[x]->data[y+3] & (1 << (bpos + 1)))
					byte |= 1 << 7;

				set_bytes(out_buffer, byte);
				send_buffer((uint8_t*)out_buffer, sizeof(out_buffer),ptr);
			}
		}
		SEND_CONST_STRING("\r\n", ptr);
	}
}
