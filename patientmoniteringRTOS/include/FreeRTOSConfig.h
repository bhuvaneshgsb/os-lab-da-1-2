#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*-----------------------------------------------------------
 * Basic FreeRTOS configuration
 *----------------------------------------------------------*/

#define configUSE_PREEMPTION                    1
#define configUSE_IDLE_HOOK                    0
#define configUSE_TICK_HOOK                    0

#define configCPU_CLOCK_HZ                     ( ( unsigned long ) 1000000 )
#define configTICK_RATE_HZ                     ( ( TickType_t ) 1000 )

#define configMAX_PRIORITIES                    7
#define configMINIMAL_STACK_SIZE                128
#define configTOTAL_HEAP_SIZE                  ( ( size_t ) ( 20 * 1024 ) )

#define configMAX_TASK_NAME_LEN                 16

#define configTICK_TYPE_WIDTH_IN_BITS          TICK_TYPE_WIDTH_32_BITS

#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1

#define configUSE_QUEUE_SETS                    1
#define configQUEUE_REGISTRY_SIZE               10

#define configCHECK_FOR_STACK_OVERFLOW          2
#define configUSE_MALLOC_FAILED_HOOK            1

#define configUSE_TRACE_FACILITY                1
#define configUSE_STATS_FORMATTING_FUNCTIONS    1

/* Software timers */

#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               ( configMAX_PRIORITIES - 1 )
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            256

/* API functions */

#define INCLUDE_vTaskDelay                     1
#define INCLUDE_vTaskDelayUntil               1
#define INCLUDE_vTaskDelete                    1
#define INCLUDE_vTaskSuspend                   1
#define INCLUDE_xTaskGetSchedulerState         1
#define INCLUDE_uxTaskPriorityGet              1
#define INCLUDE_vTaskPrioritySet               1

/*-----------------------------------------------------------
 * Windows simulator
 *----------------------------------------------------------*/

#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0

#endif