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

#include <device.h>
#include <math.h>
#include <stdio.h>

#include "ZCDtoPWM.h"
#include "autotune.h"
#include "helper/brailledraw.h"
#include "helper/teslaterm.h"
#include "cli_common.h"
#include "interrupter.h"
#include "tasks/tsk_analog.h"
#include "tasks/tsk_overlay.h"

#define FREQ 0
#define CURR 1

#define TTERM_HEIGHT 300L
#define TTERM_WIDTH 400L

#define OFFSET_X 20
#define OFFSET_Y 20

void autotune_draw_d(uint8_t port){
    uint16_t f;
    send_chart_line(OFFSET_X, TTERM_HEIGHT+OFFSET_Y, TTERM_WIDTH+OFFSET_X, TTERM_HEIGHT+OFFSET_Y, TT_COLOR_WHITE, port);
    send_chart_line(OFFSET_X, OFFSET_Y, OFFSET_X, TTERM_HEIGHT+OFFSET_Y, TT_COLOR_WHITE, port);

	for (f = 0; f <= TTERM_HEIGHT; f += 25) {
		send_chart_line(OFFSET_X, f+OFFSET_Y, OFFSET_X-10, f+OFFSET_Y, TT_COLOR_WHITE, port);
        send_chart_line(OFFSET_X, f+OFFSET_Y, TTERM_WIDTH+OFFSET_X, f+OFFSET_Y, TT_COLOR_GRAY, port);
	}

}



uint16_t run_adc_sweep(uint16_t F_min, uint16_t F_max, uint16_t pulsewidth, uint8_t channel, uint8_t delay, uint8_t port) {
    uint16 freq_response[128][2]; //[frequency value, amplitude]
	CT_MUX_Select(channel);
	//units for frequency are 0.1kHz (so 1000 = 100khz).  Pulsewidth in uS
	uint16 f;
	uint8 n;
	uint16 original_freq;
	uint8 original_lock_cycles;
	char buffer[100];
	float current_buffer = 0;

	braille_clear();

	if (F_min > F_max) {
		return 0;
	}
	//store the original setting, restore it after the sweep
	original_freq = configuration.start_freq;
	original_lock_cycles = configuration.start_cycles;

	//max_response = 0;
	for (f = 0; f < 128; f++) {
		freq_response[f][CURR] = 0;
	}

	//create frequency table based on F_max, F_min, 128 values, [f],[0] holds frequency, [f],[1] holds amplitude response

	for (f = 0; f < 128; f++) {
		current_buffer = 0;
		//this generates the requested trial frequency for the table entry, starting at Fmin and working our way up to Fmax
		freq_response[f][FREQ] = (((F_max - F_min) * (f + 1)) >> 7) + F_min;

		configuration.start_freq = freq_response[f][FREQ];
		configuration.start_cycles = 120;
		configure_ZCD_to_PWM();

		for (n = 0; n < configuration.autotune_s; n++) {

			//command a one shot

			interrupter_oneshot(pulsewidth, 127);

			vTaskDelay(delay / portTICK_PERIOD_MS);

			current_buffer += CT1_Get_Current_f(channel);
		}
		freq_response[f][CURR] = round(current_buffer / (float)configuration.autotune_s * 10);
		sprintf(buffer, "Frequency: %i00Hz Current: %4i,%iA     \r", freq_response[f][FREQ], freq_response[f][CURR] / 10, freq_response[f][CURR] % 10);
		send_string(buffer, port);
	}
	send_string("\r\n", port);

	//restore original frequency
	configuration.start_freq = original_freq;
	configuration.start_cycles = original_lock_cycles;
	configure_ZCD_to_PWM();

	CT_MUX_Select(CT_PRIMARY);

	//search max current
	uint16_t max_curr = 0;
	uint8_t max_curr_num = 0;
	for (f = 0; f < 128; f++) {
		if (freq_response[f][CURR] > max_curr) {
			max_curr = freq_response[f][CURR];
			max_curr_num = f;
		}
	}

	//Draw Diagram

    if(get_overlay_mode(port)== TERM_MODE_VT100){
	    braille_line(0, 63, 127, 63);
	    braille_line(0, 0, 0, 63);

    	for (f = 0; f < PIX_HEIGHT; f += 4) {
    		braille_line(0, f, 2, f);
    	}
    	for (f = 0; f < PIX_WIDTH; f += 4) {
    		braille_line(f, PIX_HEIGHT - 1, f, PIX_HEIGHT - 3);
    	}

    	for (f = 1; f < 128; f++) {
    		braille_line(f - 1, ((PIX_HEIGHT - 1) - ((PIX_HEIGHT - 1) * freq_response[f - 1][CURR]) / max_curr), f, ((PIX_HEIGHT - 1) - ((PIX_HEIGHT - 1) * freq_response[f][CURR]) / max_curr));
    	}
    	braille_draw(port);
    	for (f = 0; f < PIX_WIDTH / 2; f += 4) {
    		if (!f) {
    			sprintf(buffer, "%i", freq_response[f * 2][FREQ]);
    			send_string(buffer, port);
    		} else {
    			sprintf(buffer, "%4i", freq_response[f * 2][FREQ]);
    			send_string(buffer, port);
    		}
    	}
    }else{

        float step_w = (float)TTERM_WIDTH/128;
        
        float x_val_1;
        float x_val_2;
        autotune_draw_d(port);
        
        //Draw x grid
        for (f = 0; f < 128; f+=8) {
            x_val_1 = f*step_w;
            send_chart_line(x_val_1+OFFSET_X, OFFSET_Y, x_val_1+OFFSET_X, TTERM_HEIGHT+OFFSET_Y, TT_COLOR_GRAY, port);
    	}
        f=127;
        x_val_1 = f*step_w;
        send_chart_line(x_val_1+OFFSET_X, OFFSET_Y, x_val_1+OFFSET_X, TTERM_HEIGHT+OFFSET_Y, TT_COLOR_GRAY, port);
        
        //Draw line
    	for (f = 0; f < 127; f++) {
            x_val_1 = f*step_w;
            x_val_2 = (f+1)*step_w;
    		send_chart_line(x_val_1+OFFSET_X, ((TTERM_HEIGHT - 1) - ((TTERM_HEIGHT - 1) * freq_response[f][CURR]) / max_curr)+OFFSET_Y, x_val_2+OFFSET_X, ((TTERM_HEIGHT - 1) - ((TTERM_HEIGHT - 1) * freq_response[f+1][CURR]) / max_curr)+OFFSET_Y, TT_COLOR_GREEN, port);
    	}
        
        //Draw text
    	for (f = 0; f < 128; f+=8) {
            x_val_1 = f*step_w;
            sprintf(buffer, "%i,%i", freq_response[f][FREQ]/10,freq_response[f][FREQ]%10);
    		send_chart_text_center(x_val_1+OFFSET_X,OFFSET_Y+TTERM_HEIGHT+20,TT_COLOR_WHITE,8,buffer,port);
            send_chart_line(x_val_1+OFFSET_X, TTERM_HEIGHT+OFFSET_Y, x_val_1+OFFSET_X, TTERM_HEIGHT +10+OFFSET_Y, TT_COLOR_WHITE, port);
    	}
        f=127;
        x_val_1 = f*step_w;
        sprintf(buffer, "%i,%i", freq_response[f][FREQ]/10,freq_response[f][FREQ]%10);
    	send_chart_text_center(x_val_1+OFFSET_X,OFFSET_Y+TTERM_HEIGHT+20,TT_COLOR_WHITE,8,buffer,port);
        send_chart_line(x_val_1+OFFSET_X, TTERM_HEIGHT+OFFSET_Y, x_val_1+OFFSET_X, TTERM_HEIGHT +10+OFFSET_Y, TT_COLOR_WHITE, port);
        x_val_1+=15;
        send_chart_text(x_val_1+OFFSET_X,OFFSET_Y+TTERM_HEIGHT+20,TT_COLOR_WHITE,8,"khz",port);
        
        
        
    }
    
	sprintf(buffer, "\r\nFound Peak at: %i00Hz\r\n", freq_response[max_curr_num][FREQ]);
	send_string(buffer, port);
    return freq_response[max_curr_num][FREQ];
}

/* [] END OF FILE */
