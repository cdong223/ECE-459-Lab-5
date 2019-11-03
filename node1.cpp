#include <nrk.h>
#include <include.h>
#include <ulib.h>
#include <stdio.h>
#include <hal.h>
#include <nrk_error.h>
#include <nrk_timer.h>
#include <nrk_stack_check.h>
#include <nrk_stats.h>
#include <string.h>
#include "mbed.h"
#include "basic_rf.h"
#include "bmac.h"

//#define SEND
#define RECEIVE
#define PRINT

void rx_task (void);
void tx_task (void);
void lit_task (void);

nrk_task_type PRINT_TASK;
NRK_STK print_task_stack[NRK_APP_STACKSIZE];
void print_task (void);

void nrk_create_taskset ();

char tx_buf[RF_MAX_PAYLOAD_SIZE];
char rx_buf[RF_MAX_PAYLOAD_SIZE];

RF_RX_INFO rfRxInfo;
RF_TX_INFO rfTxInfo;

uint8_t rx_buf_empty = 1;

// Counter for number of detected packets that have valid data
static uint16_t rxContentSuccess = 0;
// Counter for number of detected packets from the preamble perspective, regardless of contents
static uint16_t mrfRxISRcallbackCounter = 0;

static int sentPackets = 0;
DigitalIn sensor(p30);
static int hit_flag;
//static int timeout_flag;
//static int count;

// Count number of detected packets and toggle a pin for each packet
void mrfIsrCallback()
{
	//nrk_led_toggle(ORANGE_LED);
	mrfRxISRcallbackCounter++;
	rx_buf_empty = 0;
}
void SensorInterrupt(){
	hit_flag = 1;
}

int main(void)
{
	nrk_setup_ports();
	rfRxInfo.pPayload = rx_buf;
	rfRxInfo.max_length = RF_MAX_PAYLOAD_SIZE;
	rf_init(&rfRxInfo, RADIO_CHANNEL, 0xFFFF, MAC_ADDR); //RX struct, channel, panId, myAddr
	rf_auto_ack_enable(); // enable if IEEE 802.15.4 ack packets are requested
	//rf_auto_ack_disable();
	nrk_init();
	nrk_create_taskset();
  nrk_start();

	sensor.rise(&SensorInterrupt);
	while(1){
		if(!rx_buf_empty){
			rx_task();
		}
		if(hit_flag){
			tx_task();
		}
	}

	return 0;
}

void rx_task ()
{
  if(!rx_buf_empty)
	{
		if (strncmp(rx_buf, "lit", 41) == 0)
		{
			nrk_led_toggle(ORANGE_LED);
			nrk_led_toggle(BLUE_LED);
			nrk_led_toggle(GREEN_LED);
			nrk_led_toggle(RED_LED);
			rxContentSuccess++;
		}
		memset(rx_buf, 0, strlen(rx_buf));
		rx_buf_empty = 1;
	}
}

void tx_task ()
{
	rfTxInfo.destAddr = 0xF00B; // destination node address
	if(hit_flag) {
		hit_flag = 0;
		sprintf (tx_buf, "success");
		rfTxInfo.pPayload=tx_buf;
		rfTxInfo.length=strlen(tx_buf);
		rf_rx_off();
		rf_tx_packet(&rfTxInfo);
		rf_rx_on();
		nrk_led_toggle(ORANGE_LED);
		nrk_led_toggle(BLUE_LED);
		nrk_led_toggle(GREEN_LED);
		nrk_led_toggle(RED_LED);
		sentPackets++;
		memset(tx_buf, 0, strlen(tx_buf));
	}
}

void lit_task () {
}

void print_task () {
	while(1) {
		printf("Received: %d; Sent: %d\n", rxContentSuccess, sentPackets);
		//printf("Received: %s(%d); Sent: %s(%d)\n", rx_buf, rxContentSuccess, tx_buf, sentPackets);
		//memset(rx_buf, 0, strlen(rx_buf));
		//memset(tx_buf, 0, strlen(tx_buf));
		nrk_wait_until_next_period();
	}
}

void nrk_create_taskset ()
{
	// Separately activate Tx task on one node, and Rx task on the other
	// or both if congestion operation is desirable
	PRINT_TASK.task = print_task;
	nrk_task_set_stk( &PRINT_TASK, print_task_stack, NRK_APP_STACKSIZE);
	PRINT_TASK.prio = 2;
  PRINT_TASK.FirstActivation = TRUE;
  PRINT_TASK.Type = BASIC_TASK;
  PRINT_TASK.SchType = PREEMPTIVE;
  PRINT_TASK.period.secs = 0;
  PRINT_TASK.period.nano_secs = 100*NANOS_PER_MS;
  PRINT_TASK.cpu_reserve.secs = 0;
  PRINT_TASK.cpu_reserve.nano_secs = 0;
  PRINT_TASK.offset.secs = 0;
  PRINT_TASK.offset.nano_secs = 0;
	#ifdef PRINT
		nrk_activate_task (&PRINT_TASK);
	#endif
}
