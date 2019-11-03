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

void rx_task (void);
void tx_task (void);

nrk_task_type PRINT_TASK;
NRK_STK print_task_stack[NRK_APP_STACKSIZE];
void print_task (void);

void nrk_create_taskset ();

char tx_buf[RF_MAX_PAYLOAD_SIZE];
char rx_buf[RF_MAX_PAYLOAD_SIZE];

RF_RX_INFO rfRxInfo;
RF_TX_INFO rfTxInfo;

uint8_t rx_buf_empty = 1;

static uint16_t rxContentSuccess = 0;
static uint16_t mrfRxISRcallbackCounter = 0;

static int sentPackets = 0;
static int score;
static int feedback_flag = 1;

// Count number of detected packets and toggle a pin for each packet
void mrfIsrCallback()
{
	nrk_led_toggle(ORANGE_LED);
	mrfRxISRcallbackCounter++;
	rx_buf_empty = 0;
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

	while(sentPackets < 10){
		if(feedback_flag){
			wait(3); //wait 3 seconds before sending out next 'mole'
			tx_task();
		}
		if(!rx_buf_empty){
				rx_task();
		}
	}

	return 0;
}

void rx_task ()
{
  // Wait until an RX packet is received
  if(!rx_buf_empty)
	{
		if (strncmp(rx_buf, "success", 41) == 0)
		{
			score++;
			feedback_flag = 1;
			rxContentSuccess++;
		}
		memset(rx_buf,0,strlen(rx_buf));
		rx_buf_empty = 1;
	}
}

void tx_task ()
{
	//int packetsToSend = 10;
	if (feedback_flag) {
		sprintf (tx_buf, "lit");
		rfTxInfo.pPayload=tx_buf;
		rfTxInfo.length=strlen(tx_buf);
		rfTxInfo.destAddr = 0x000B; // destination node address
		rf_rx_off();
		rf_tx_packet(&rfTxInfo);
		rf_rx_on();
		nrk_led_toggle(ORANGE_LED);
		//packetsToSend--;
		sentPackets++;
		feedback_flag = 0;
	}
}

void print_task () {
	while(1) {
		printf("Sent: %d; Received: %d; Score: %d\n", sentPackets, rxContentSuccess, score);
		//printf("Received: %s(%d); Sent: %d; Score: %d\n", rx_buf, rxContentSuccess, sentPackets, score);
		//memset(rx_buf, 0, strlen(rx_buf));
		nrk_wait_until_next_period();
	}
}

void nrk_create_taskset ()
{
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
