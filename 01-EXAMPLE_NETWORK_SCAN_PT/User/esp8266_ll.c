/**	
 * |----------------------------------------------------------------------
 * | Copyright (c) 2016 Tilen Majerle
 * |  
 * | Permission is hereby granted, free of charge, to any person
 * | obtaining a copy of this software and associated documentation
 * | files (the "Software"), to deal in the Software without restriction,
 * | including without limitation the rights to use, copy, modify, merge,
 * | publish, distribute, sublicense, and/or sell copies of the Software, 
 * | and to permit persons to whom the Software is furnished to do so, 
 * | subject to the following conditions:
 * | 
 * | The above copyright notice and this permission notice shall be
 * | included in all copies or substantial portions of the Software.
 * | 
 * | THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * | EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * | OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * | AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * | HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * | WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * | FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * | OTHER DEALINGS IN THE SOFTWARE.
 * |----------------------------------------------------------------------
 */
#include "esp8266_ll.h"
#include "esp8266.h"

/* Include platform dependant libraries */
#include "stm32fxxx_hal.h"
#include "tm_stm32_usart.h"
#include "tm_stm32_usart_dma.h"
#include "tm_stm32_delay.h"
#include "tm_stm32_gpio.h"

#if ESP_RTOS
osMutexId id;
#endif /* ESP_RTOS */

uint8_t ESP_LL_Callback(ESP_LL_Control_t ctrl, void* param, void* result) {
    switch (ctrl) {
        case ESP_LL_Control_Init: {                 /* Initialize low-level part of communication */
            ESP_LL_t* LL = (ESP_LL_t *)param;       /* Get low-level value from callback */
            
            /************************************/
            /*  Device specific initialization  */
            /************************************/
            TM_USART_Init(USART1, TM_USART_PinsPack_1, LL->Baudrate);
            TM_GPIO_Init(GPIOA, GPIO_PIN_0, TM_GPIO_Mode_OUT, TM_GPIO_OType_PP, TM_GPIO_PuPd_UP, TM_GPIO_Speed_Low);
            TM_GPIO_SetPinHigh(GPIOA, GPIO_PIN_0);
            
            if (result) {
                *(uint8_t *)result = 0;             /* Successfully initialized */
            }
            return 1;                               /* Return 1 = command was processed */
        }
        case ESP_LL_Control_Send: {
            ESP_LL_Send_t* send = (ESP_LL_Send_t *)param;   /* Get send parameters */
            
            /* Send actual data to UART */
            TM_USART_Send(USART1, (uint8_t *)send->Data, send->Count);  /* Send actual data */
            
            if (result) {
                *(uint8_t *)result = 0;             /* Successfully send */
            }
            return 1;                               /* Command processed */
        }
        case ESP_LL_Control_SetReset: {             /* Set reset value */
            uint8_t state = *(uint8_t *)param;      /* Get state packed in uint8_t variable */
            if (state == ESP_RESET_SET) {           /* Check state value */
                TM_GPIO_SetPinLow(GPIOA, GPIO_PIN_0);
            } else {
                TM_GPIO_SetPinHigh(GPIOA, GPIO_PIN_0);
            }
            return 1;                               /* Command has been processed */
        }
        case ESP_LL_Control_SetRTS: {               /* Set RTS value */
            uint8_t state = *(uint8_t *)param;      /* Get state packed in uint8_t variable */
            (void)state;                            /* Prevent compiler warnings */
            return 1;                               /* Command has been processed */
        }
#if ESP_RTOS
        case ESP_LL_Control_SYS_Create: {           /* Create system synchronization object */
            ESP_RTOS_SYNC_t* Sync = (ESP_RTOS_SYNC_t *)param;   /* Get pointer to sync object */
            id = osMutexCreate(Sync);               /* Create mutex */
            
            if (result) {
                *(uint8_t *)result = id == NULL;    /*!< Set result value */
            }
            return 1;                               /* Command processed */
        }
        case ESP_LL_Control_SYS_Delete: {           /* Delete system synchronization object */
            ESP_RTOS_SYNC_t* Sync = (ESP_RTOS_SYNC_t *)param;   /* Get pointer to sync object */
            osMutexDelete(Sync);                    /* Delete mutex object */
            
            if (result) {
                *(uint8_t *)result = id == NULL;    /*!< Set result value */
            }
            return 1;                               /* Command processed */
        }
        case ESP_LL_Control_SYS_Request: {          /* Request system synchronization object */
            ESP_RTOS_SYNC_t* Sync = (ESP_RTOS_SYNC_t *)param;   /* Get pointer to sync object */
            (void)Sync;                             /* Prevent compiler warnings */
            
            *(uint8_t *)result = osMutexWait(id, 1000) == osOK ? 0 : 1; /* Set result according to response */
            return 1;                               /* Command processed */
        }
        case ESP_LL_Control_SYS_Release: {          /* Release system synchronization object */
            ESP_RTOS_SYNC_t* Sync = (ESP_RTOS_SYNC_t *)param;   /* Get pointer to sync object */
            (void)Sync;                             /* Prevent compiler warnings */
            
            *(uint8_t *)result = osMutexRelease(id) == osOK ? 0 : 1;    /* Set result according to response */
            return 1;                               /* Command processed */
        }
#endif /* ESP_RTOS */
        default: 
            return 0;
    }
}

/* USART receive interrupt handler */
void TM_USART1_ReceiveHandler(uint8_t ch) {
	/* Send received character to ESP stack */
	ESP_DataReceived(&ch, 1);
}
