/* ###################################################################
**     Filename    : main.c
**     Processor   : S32K1xx
**     Abstract    :
**         Main module.
**         This module contains user's application code.
**     Settings    :
**     Contents    :
**         No public methods
**
** ###################################################################*/
/*!
** @file main.c
** @version 01.00
** @brief
**         Main module.
**         This module contains user's application code.
*/         
/*!
**  @addtogroup main_module main module documentation
**  @{
*/         
/* MODULE main */


/* Including necessary module. Cpu.h contains other modules needed for compiling.*/
#include "Cpu.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "interrupt_manager.h"
#include "clock_manager.h"
#include "clockMan1.h"
#include "pin_mux.h"

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

volatile int exit_code = 0;

void delay(volatile int cycles);
//static void pwm(void *pvParameters);

//#define pwmsize 90
//#define pwmpro  1
//TaskHandle_t pxpwmtask;

void rxCallback(void *driverState, uart_event_t event, void *userData);
#define uartsize 90
#define uartpro  2
TaskHandle_t pxuarttask;

// void delay(volatile int cycles)
//  {
//  	/* Delay function - do nothing for a number of cycles */
//  	while(cycles--);
//  }
//  /* The queue used by both tasks. */
//  static void pwm(void *pvParameters)
//  {
//  	(void)pvParameters;
//  	uint32_t duty = 0;
//      bool increaseDuty = true;
//      uint8_t channel = pwm_pal1Configs.pwmChannels[0].channel;
//      while(1)
//  	 {
//  		 PWM_UpdateDuty(&pwm_pal1Instance, channel, duty);
//  		 if (increaseDuty == true)
//  			{
//  			   duty++;
//  			   if (duty > 4999U)
//  				   increaseDuty = false;
//  			}
//  		 else
//  			{
//  			   duty--;
//  			   if (duty < 1U)
//  				   increaseDuty = true;
//  			}
//  	  delay(1500U);
//  	 }
//  }

#define welcomeMsg "This example is an simple echo using LPUART\r\n\
it will send back any character you send to it.\r\n\
The board will greet you if you send 'Hello Board'\r\
\nNow you can begin typing:\r\n"

/* Error message displayed at the console, in case data is received erroneously */
#define errorMsg "An error occurred! The application will stop!\r\n"

/* Buffer used to receive data from the console */
#define BUFFER_SIZE 256U
//#define TIMEOUT     100U
uint8_t buffer[BUFFER_SIZE];
uint8_t bufferIdx;
void rxCallback(void *driverState, uart_event_t event, void *userData)
{
    /* Unused parameters */
    (void)driverState;
    (void)userData;

    /* Check the event type */
    if (event == UART_EVENT_RX_FULL)
    {
        /* The reception stops when newline is received or the buffer is full */
        if ((buffer[bufferIdx] != '\n') && (bufferIdx != (BUFFER_SIZE - 2U)))
        {
            /* Update the buffer index and the rx buffer */
            bufferIdx++;
            LPUART_DRV_SetRxBuffer(INST_LPUART1, &buffer[bufferIdx], 1U);
        }
    }
}

static void uart_task(void *pvParameters)
{
	(void)pvParameters;
	 status_t status;
	 uint32_t bytesRemaining;
   LPUART_DRV_Init(INST_LPUART1, &lpuart1_State, &lpuart1_InitConfig0);
   LPUART_DRV_InstallRxCallback(INST_LPUART1,&rxCallback, NULL);
   //LPUART_DRV_SendDataBlocking(INST_LPUART1, (uint8_t *)welcomeMsg, strlen(welcomeMsg), TIMEOUT);
   LPUART_DRV_SendData(INST_LPUART1,(uint8_t *)welcomeMsg, strlen(welcomeMsg));
   while (1)
	 {
		 LPUART_DRV_ReceiveData(INST_LPUART1, buffer, 1U);
		 /* Wait for transfer to be completed */
		 while(LPUART_DRV_GetReceiveStatus(INST_LPUART1, &bytesRemaining) == STATUS_BUSY);
		 /* Check the status */
		 status = LPUART_DRV_GetReceiveStatus(INST_LPUART1, &bytesRemaining);
		 if (status != STATUS_SUCCESS)
		 {
			 /* If an error occurred, send the error message and exit the loop */
			 //LPUART_DRV_SendDataBlocking(INST_LPUART1, (uint8_t *)errorMsg, strlen(errorMsg), TIMEOUT);
			 LPUART_DRV_SendData(INST_LPUART1, (uint8_t *)errorMsg, strlen(errorMsg));
			 break;
		 }
		 /* Append string terminator to the received data */
		 bufferIdx++;
		 buffer[bufferIdx] = 0U;
		 /* If the received string is "Hello Board", send back "Hello World" */
		 if(strcmp((char *)buffer, "Hello Board\n") >= 0)
		 {
			strcpy((char *)buffer, "Hello World\n");
		 }
		 /* Send the received data back */
		 LPUART_DRV_SendDataBlocking(INST_LPUART1, buffer, bufferIdx, TIMEOUT);
		 LPUART_DRV_SendData(INST_LPUART1,buffer, bufferIdx);
		 /* Reset the buffer index to start a new reception */
		 bufferIdx = 0U;
	 }
}

int main(void)
{
  #ifdef PEX_RTOS_INIT
    PEX_RTOS_INIT();                   /* Initialization of the selected RTOS. Macro is defined by the RTOS component. */
  #endif
  /*** End of Processor Expert internal initialization.                    ***/
	  CLOCK_SYS_Init(g_clockManConfigsArr, CLOCK_MANAGER_CONFIG_CNT,
					 g_clockManCallbacksArr, CLOCK_MANAGER_CALLBACK_CNT);
	  CLOCK_SYS_UpdateConfiguration(0U, CLOCK_MANAGER_POLICY_AGREEMENT);
	  PINS_DRV_Init(NUM_OF_CONFIGURED_PINS, g_pin_mux_InitConfigArr);

//	xTaskCreate(pwm,"pwm",pwmsize,NULL,pwmpro,&pxpwmtask);
	xTaskCreate(uart_task,"uart_task",uartsize,NULL,uartpro,&pxuarttask);
	vTaskStartScheduler();

  #ifdef PEX_RTOS_START
    PEX_RTOS_START();                  /* Startup of the selected RTOS. Macro is defined by the RTOS component. */
  #endif
  /*** RTOS startup code. Macro PEX_RTOS_START is defined by the RTOS component. DON'T MODIFY THIS CODE!!! ***/
  #ifdef PEX_RTOS_START
    PEX_RTOS_START();                  /* Startup of the selected RTOS. Macro is defined by the RTOS component. */
  #endif
  /*** End of RTOS startup code.  ***/
  /*** Processor Expert end of main routine. DON'T MODIFY THIS CODE!!! ***/
  for(;;) {
    if(exit_code != 0) {
      break;
    }
  }
  return exit_code;
  /*** Processor Expert end of main routine. DON'T WRITE CODE BELOW!!! ***/
} /*** End of main routine. DO NOT MODIFY THIS TEXT!!! ***/

/* END main */
/*!
** @}
*/
/*
** ###################################################################
**
**     This file was created by Processor Expert 10.1 [05.21]
**     for the NXP S32K series of microcontrollers.
**
** ###################################################################
*/
