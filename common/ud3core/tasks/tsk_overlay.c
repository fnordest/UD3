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

#include "cyapicallbacks.h"
#include <cytypes.h>

#include "tsk_overlay.h"

/* RTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "helper/teslaterm.h"

/* ------------------------------------------------------------------------ */
/*
 * Place user included headers, defines and task global data in the
 * below merge region section.
 */
/* `#START USER_INCLUDE SECTION` */
#include "cli_common.h"
#include "telemetry.h"
#include "charging.h"
#include "tsk_analog.h"
#include "tsk_uart.h"
#include <project.h>
#include <stdio.h>
#include "ZCDtoPWM.h"

/* `#END` */
/* ------------------------------------------------------------------------ */
/*
 * User defined task-local code that is used to process commands, control
 * operations, and/or generrally do stuff to make the taks do something
 * meaningful.
 */
/* `#START USER_TASK_LOCAL_CODE` */
uint8_t chart;
uint8_t term_mode[NUM_PORTS];

void tsk_overlay_chart_stop(){
    chart=0;
}
void tsk_overlay_chart_start(){
    chart=1;
}

uint8_t set_overlay_mode(uint8_t mode, uint8_t port){
    if(port < NUM_PORTS){
        term_mode[port] = mode;  
        return mode;
    }else{
        return -1;
    }
}

uint8_t get_overlay_mode(uint8_t port){
    if(port < NUM_PORTS){
        return term_mode[port];   
    }
    return -1;
}

void show_overlay_100ms(uint8_t port) {

    if(term_mode[port] == TERM_MODE_VT100){
        char buffer[50];
    	Term_Save_Cursor(port);
    	send_string("\033[?25l", port);

    	uint8_t row_pos = 1;
    	uint8_t col_pos = 90;
    	//Term_Erase_Screen(port);
    	Term_Box(row_pos, col_pos, row_pos + 11, col_pos + 25, port);
    	Term_Move_Cursor(row_pos + 1, col_pos + 1, port);
    	sprintf(buffer, "Bus Voltage:       %4iV", telemetry.bus_v);
    	send_string(buffer, port);

    	Term_Move_Cursor(row_pos + 2, col_pos + 1, port);
    	sprintf(buffer, "Battery Voltage:   %4iV", telemetry.batt_v);
    	send_string(buffer, port);

    	Term_Move_Cursor(row_pos + 3, col_pos + 1, port);
    	sprintf(buffer, "Temp 1:          %4i *C", telemetry.temp1);
    	send_string(buffer, port);

    	Term_Move_Cursor(row_pos + 4, col_pos + 1, port);
    	sprintf(buffer, "Temp 2:          %4i *C", telemetry.temp2);
    	send_string(buffer, port);

    	Term_Move_Cursor(row_pos + 5, col_pos + 1, port);
    	send_string("Bus status: ", port);

    	switch (telemetry.bus_status) {
    	case BUS_READY:
    		send_string("       Ready", port);
    		break;
    	case BUS_CHARGING:
    		send_string("    Charging", port);
    		break;
    	case BUS_OFF:
    		send_string("         OFF", port);
    		break;
    	}

    	Term_Move_Cursor(row_pos + 6, col_pos + 1, port);
    	sprintf(buffer, "Average power:     %4iW", telemetry.avg_power);
    	send_string(buffer, port);

    	Term_Move_Cursor(row_pos + 7, col_pos + 1, port);
    	sprintf(buffer, "Average Current: %4i.%iA", telemetry.batt_i / 10, telemetry.batt_i % 10);
    	send_string(buffer, port);

    	Term_Move_Cursor(row_pos + 8, col_pos + 1, port);
    	sprintf(buffer, "Primary Current:   %4iA", telemetry.primary_i);
    	send_string(buffer, port);
        
        Term_Move_Cursor(row_pos + 9, col_pos + 1, port);
    	sprintf(buffer, "MIDI voices:         %1i/4", telemetry.midi_voices);
    	send_string(buffer, port);
        
        Term_Move_Cursor(row_pos + 10, col_pos + 1, port);
    	sprintf(buffer, "DAC Value:           %3i", ct1_dac_val[0]);
    	send_string(buffer, port);

    	Term_Restore_Cursor(port);
    	send_string("\033[?25h", port);
    
    }else{
        #if GAUGE0_SLOW==0
        send_gauge(0, GAUGE0_VAR, port);
        #endif
        #if GAUGE1_SLOW==0
        send_gauge(1, GAUGE1_VAR, port);
        #endif
        #if GAUGE2_SLOW==0
        send_gauge(2, GAUGE2_VAR, port);
        #endif
        #if GAUGE3_SLOW==0
        send_gauge(3, GAUGE3_VAR, port);
        #endif
        #if GAUGE4_SLOW==0
        send_gauge(4, GAUGE4_VAR, port);
        #endif
        #if GAUGE5_SLOW==0
        send_gauge(5, GAUGE5_VAR, port);
        #endif
        #if GAUGE6_SLOW==0
        send_gauge(6, GAUGE6_VAR, port);
        #endif
        
        if(chart){
            send_chart(0, CHART0_VAR, port);
            send_chart(1, CHART1_VAR, port);
            send_chart(2, CHART2_VAR, port);
            send_chart(3, CHART3_VAR, port);
        
            send_chart_draw(port);
        }
        
    }
	
}

void show_overlay_400ms(uint8_t port) {
    if(term_mode[port] != TERM_MODE_VT100){
        #if GAUGE0_SLOW==1
        send_gauge(0, GAUGE0_VAR, port);
        #endif
        #if GAUGE1_SLOW==1
        send_gauge(1, GAUGE1_VAR, port);
        #endif
        #if GAUGE2_SLOW==1
        send_gauge(2, GAUGE2_VAR, port);
        #endif
        #if GAUGE3_SLOW==1
        send_gauge(3, GAUGE3_VAR, port);
        #endif
        #if GAUGE4_SLOW==1
        send_gauge(4, GAUGE4_VAR, port);
        #endif
        #if GAUGE5_SLOW==1
        send_gauge(5, GAUGE5_VAR, port);
        #endif
        #if GAUGE6_SLOW==1
        send_gauge(6, GAUGE6_VAR, port);
        #endif

        send_status(telemetry.bus_status!=BUS_OFF,
                    tr_running!=0,
                    configuration.ps_scheme!=AC_NO_RELAY_BUS_SCHEME, port);
    }
}



/* `#END` */
/* ------------------------------------------------------------------------ */
/*
 * This is the main procedure that comprises the task.  Place the code required
 * to preform the desired function within the merge regions of the task procedure
 * to add functionality to the task.
 */
void tsk_overlay_TaskProc(void *pvParameters) {
	/*
	 * Add and initialize local variables that are allocated on the Task stack
	 * the the section below.
	 */
	/* `#START TASK_VARIABLES` */
    
    uint8_t cnt=0;
    
	/* `#END` */

	/*
	 * Add the task initialzation code in the below merge region to be included
	 * in the task.
	 */
	/* `#START TASK_INIT_CODE` */

	/* `#END` */

	for (;;) {
		/* `#START TASK_LOOP_CODE` */
        xSemaphoreTake(block_term[(uint32_t)pvParameters], portMAX_DELAY);
        show_overlay_100ms((uint32_t)pvParameters);
        if(cnt<3){
            cnt++;
        }else{
            cnt=0;
            show_overlay_400ms((uint32_t)pvParameters);
        }
        xSemaphoreGive(block_term[(uint32_t)pvParameters]);
        
		/* `#END` */
        if(term_mode[(uint32_t)pvParameters]==TERM_MODE_VT100){
		    vTaskDelay(500 / portTICK_PERIOD_MS);
        }else{
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
	}
}
