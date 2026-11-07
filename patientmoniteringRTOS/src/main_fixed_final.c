#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "vitals.h"
#include "severity.h"
#include "database.h"
#include "prediction.h"
/* ---------------------------------------------------------
 * Shared patient vital signs
 * --------------------------------------------------------- */

static VitalSigns patientVitals =
{
    .heart_rate = 75,
    .spo2 = 98,
    .temperature = 36.7f
};

/* Mutex protects shared vital-sign data */
static SemaphoreHandle_t vitalsMutex;
static SeverityLevel currentSeverity = SEVERITY_NORMAL;
static int recoveryCount = 0;
static SeverityLevel lastAlertSeverity = SEVERITY_NORMAL;
static int lastStoredSchedulerPriority = -1;
static int lastStoredSamplingInterval = -1;

/*
 * Automatic patient-condition simulator.
 *
 * Each scenario lasts 10 seconds:
 *   0 = NORMAL
 *   1 = LOW
 *   2 = MODERATE
 *   3 = HIGH
 *   4 = CRITICAL
 *   5 = RECOVERY
 *
 * The sequence repeats automatically.
 */
typedef enum
{
    SIM_NORMAL = 0,
    SIM_LOW,
    SIM_MODERATE,
    SIM_HIGH,
    SIM_CRITICAL,
    SIM_RECOVERY
} SimulationScenario;

static TickType_t simulationStartTick = 0;
static SimulationScenario lastPrintedScenario = (SimulationScenario)-1;

/*
 * This is the interval actually used by the monitoring tasks.
 * SeverityTask updates it after every scheduler decision.
 */
static volatile TickType_t monitoringInterval = pdMS_TO_TICKS(2000);
/* RTOS task handles */
static TaskHandle_t monitorTaskHandle = NULL;
static TaskHandle_t severityTaskHandle = NULL;
static TaskHandle_t emergencyTaskHandle = NULL;
/*
 * Return the monitoring interval based on
 * the current patient severity.
 */
static TickType_t getMonitoringInterval(void)
{
    return monitoringInterval;
}

/*
 * Return the current automatic simulation scenario.
 * A complete cycle takes 60 seconds.
 */
static SimulationScenario getSimulationScenario(void)
{
    TickType_t elapsedTicks = xTaskGetTickCount() - simulationStartTick;
    uint32_t elapsedSeconds =
        (uint32_t)(elapsedTicks / configTICK_RATE_HZ);

    switch ((elapsedSeconds / 10U) % 6U)
    {
        case 0: return SIM_NORMAL;
        case 1: return SIM_LOW;
        case 2: return SIM_MODERATE;
        case 3: return SIM_HIGH;
        case 4: return SIM_CRITICAL;
        default: return SIM_RECOVERY;
    }
}

/*
 * Generate realistic simulated vital signs for the current scenario.
 */
static void getSimulatedVitals(
    SimulationScenario scenario,
    uint16_t *heartRate,
    uint8_t *spo2,
    float *temperature)
{
    int variation = rand() % 3 - 1;   /* -1, 0, +1 */

    switch (scenario)
    {
        case SIM_NORMAL:
            *heartRate = (uint16_t)(75 + variation);
            *spo2 = (uint8_t)(98 + variation);
            *temperature = 36.7f + (0.1f * variation);
            break;

        case SIM_LOW:
            *heartRate = (uint16_t)(100 + variation);
            *spo2 = (uint8_t)(95 + variation);
            *temperature = 37.4f + (0.1f * variation);
            break;

        case SIM_MODERATE:
            *heartRate = (uint16_t)(110 + variation);
            *spo2 = (uint8_t)(92 + variation);
            *temperature = 38.1f + (0.1f * variation);
            break;

        case SIM_HIGH:
            *heartRate = (uint16_t)(125 + variation);
            *spo2 = (uint8_t)(88 + variation);
            *temperature = 39.0f + (0.1f * variation);
            break;

        case SIM_CRITICAL:
            *heartRate = (uint16_t)(140 + variation);
            *spo2 = (uint8_t)(82 + variation);
            *temperature = 40.0f + (0.1f * variation);
            break;

        case SIM_RECOVERY:
        default:
            *heartRate = (uint16_t)(78 + variation);
            *spo2 = (uint8_t)(97 + variation);
            *temperature = 36.8f + (0.1f * variation);
            break;
    }
}

/*
 * Print the scenario only when it changes.
 */
static void printSimulationScenario(SimulationScenario scenario)
{
    if (scenario == lastPrintedScenario)
    {
        return;
    }

    lastPrintedScenario = scenario;

    const char *name;

    switch (scenario)
    {
        case SIM_NORMAL:   name = "NORMAL";   break;
        case SIM_LOW:      name = "LOW";      break;
        case SIM_MODERATE: name = "MODERATE"; break;
        case SIM_HIGH:     name = "HIGH";     break;
        case SIM_CRITICAL: name = "CRITICAL"; break;
        default:           name = "RECOVERY"; break;
    }

    printf("\n[Simulator] Patient scenario -> %s\n", name);
}

/* ---------------------------------------------------------
 * Unified Monitor Task
 * Generates one complete vital-sign sample and updates all
 * three vital signs atomically.
 * --------------------------------------------------------- */

static void MonitorTask(void *pvParameters)
{
    (void) pvParameters;

    while (1)
    {
        uint16_t heartRate;
        uint8_t spo2;
        float temperature;
        SimulationScenario scenario = getSimulationScenario();

        printSimulationScenario(scenario);

        /* Generate ONE complete sample for this monitoring cycle. */
        getSimulatedVitals(
            scenario,
            &heartRate,
            &spo2,
            &temperature
        );

        if (xSemaphoreTake(vitalsMutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            patientVitals.heart_rate = heartRate;
            patientVitals.spo2 = spo2;
            patientVitals.temperature = temperature;

            printf("[MonitorTask] HR=%u BPM | SpO2=%u%% | Temp=%.1f C\n",
                   patientVitals.heart_rate,
                   patientVitals.spo2,
                   patientVitals.temperature);

            xSemaphoreGive(vitalsMutex);
        }
        else
        {
            printf("[MonitorTask] Could not access vital data.\n");
        }

        vTaskDelay(getMonitoringInterval());
    }
}


/* ---------------------------------------------------------
 * Severity Task
 * Checks patient condition at the adaptive monitoring interval.
 * --------------------------------------------------------- */

static void SeverityTask(void *pvParameters)
{
    (void) pvParameters;

    while (1)
    {
        VitalSigns currentVitals;
        SeverityLevel severity;
        RiskLevel predictedRisk;
        SeverityLevel effectiveSeverity;

        /*
         * Safely copy the latest vital signs.
         */
        if (xSemaphoreTake(vitalsMutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            currentVitals = patientVitals;

            xSemaphoreGive(vitalsMutex);
        }
        else
        {
            printf("[SeverityTask] Could not access vital data.\n");
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        /*
         * Calculate current patient severity.
         */
        severity = calculateSeverity(&currentVitals);
        predictedRisk = predictDeterioration(&currentVitals);

        /*
 * Convert predicted risk into a severity level.
 */
        switch (predictedRisk)
        {
            case RISK_LOW:
                effectiveSeverity = SEVERITY_LOW;
                break;

            case RISK_MODERATE:
                effectiveSeverity = SEVERITY_MODERATE;
                break;

            case RISK_HIGH:
                effectiveSeverity = SEVERITY_HIGH;
                break;

            case RISK_CRITICAL:
                effectiveSeverity = SEVERITY_CRITICAL;
                break;

            default:
                effectiveSeverity = severity;
                break;
        }

/*
 * Never reduce the current severity because of prediction.
 * Prediction can only increase monitoring urgency.
 */
if (effectiveSeverity < severity)
{
    effectiveSeverity = severity;
}

        if (severity > currentSeverity)
        {
            currentSeverity = severity;
            recoveryCount = 0;

            printf("[Severity] Condition worsening -> immediate escalation\n");
        }
        else if (severity < currentSeverity)
        {
            recoveryCount++;

            if (recoveryCount >= 3)
            {
                currentSeverity = severity;
                recoveryCount = 0;

                printf("[Severity] Condition improving -> severity reduced\n");
                if (currentSeverity != SEVERITY_CRITICAL)
                {
                    lastAlertSeverity = currentSeverity;
                }
            }
        }
    else
    {
        recoveryCount = 0;
    }


        printf("\n[SeverityTask] HR=%u BPM | SpO2=%u%% | Temp=%.1f C\n",
               currentVitals.heart_rate,
               currentVitals.spo2,
               currentVitals.temperature);

        /*
         * ----------------------------------------------------
         * ADAPTIVE RTOS SCHEDULING
         * ----------------------------------------------------
         *
         * One scheduler decision is used for both:
         *   1. FreeRTOS task priorities
         *   2. Dashboard/database scheduler information
         *
         * effectiveSeverity includes predictive risk, so a
         * prediction can increase monitoring urgency.
         */

        int schedulerPriority = 3;
        int samplingInterval = 2000;

        switch (effectiveSeverity)
        {
            case SEVERITY_NORMAL:

                schedulerPriority = 3;
                samplingInterval = 2000;

                vTaskPrioritySet(monitorTaskHandle, 3);
                vTaskPrioritySet(severityTaskHandle, 4);
                vTaskPrioritySet(emergencyTaskHandle, 7);

                printf("[Scheduler] NORMAL -> Priority=3 | Sampling=2000 ms\n");
                break;


            case SEVERITY_LOW:

                schedulerPriority = 3;
                samplingInterval = 1500;

                vTaskPrioritySet(monitorTaskHandle, 3);
                vTaskPrioritySet(severityTaskHandle, 4);
                vTaskPrioritySet(emergencyTaskHandle, 7);

                printf("[Scheduler] LOW -> Priority=3 | Sampling=1500 ms\n");
                break;


            case SEVERITY_MODERATE:

                schedulerPriority = 4;
                samplingInterval = 1000;

                vTaskPrioritySet(monitorTaskHandle, 4);
                vTaskPrioritySet(severityTaskHandle, 5);
                vTaskPrioritySet(emergencyTaskHandle, 7);

                printf("[Scheduler] MODERATE -> Priority=4 | Sampling=1000 ms\n");
                break;


            case SEVERITY_HIGH:

                schedulerPriority = 5;
                samplingInterval = 500;

                vTaskPrioritySet(monitorTaskHandle, 5);
                vTaskPrioritySet(severityTaskHandle, 6);
                vTaskPrioritySet(emergencyTaskHandle, 7);

                printf("[Scheduler] HIGH -> Priority=5 | Sampling=500 ms\n");
                break;


            case SEVERITY_CRITICAL:

                schedulerPriority = 8;
                samplingInterval = 250;

                vTaskPrioritySet(monitorTaskHandle, 6);
                vTaskPrioritySet(severityTaskHandle, 7);
                vTaskPrioritySet(emergencyTaskHandle, 8);

                printf("[Scheduler] CRITICAL -> EMERGENCY | Priority=8 | Sampling=250 ms\n");
                break;


            default:

                schedulerPriority = 3;
                samplingInterval = 2000;

                vTaskPrioritySet(monitorTaskHandle, 3);
                vTaskPrioritySet(severityTaskHandle, 4);
                vTaskPrioritySet(emergencyTaskHandle, 7);

                printf("[Scheduler] DEFAULT -> Priority=3 | Sampling=2000 ms\n");
                break;
        }

        /*
         * Apply the selected interval to the actual monitoring task.
         */
        monitoringInterval = pdMS_TO_TICKS(samplingInterval);

        printf("[Scheduler] Applied monitoring interval = %d ms\n",
               samplingInterval);


/*
 * Store the scheduler decision only when it changes.
 * This avoids unnecessary SQLite writes during CRITICAL mode.
 */
if (schedulerPriority != lastStoredSchedulerPriority ||
    samplingInterval != lastStoredSamplingInterval)
{
    if (!database_insert_scheduler(
            effectiveSeverity,
            schedulerPriority,
            samplingInterval))
    {
        printf("[Database] Failed to store scheduler information.\n");
    }
    else
    {
        lastStoredSchedulerPriority = schedulerPriority;
        lastStoredSamplingInterval = samplingInterval;
    }
}

        printf("\n");

        printf("[Prediction] Deterioration Risk = %s\n",
        riskLevelToString(predictedRisk));

/* Store current vital signs in SQLite */
        if (!database_insert_vitals(&currentVitals))
        {
            printf("[Database] Failed to store vital readings.\n");
        }

/* Store severity information in SQLite */
        if (!database_insert_severity(&currentVitals, severity))
        {
            printf("[Database] Failed to store severity.\n");
        }

        /* Store deterioration prediction in SQLite */
        if (!database_insert_prediction(&currentVitals, predictedRisk))
    {
        printf("[Database] Failed to store prediction.\n");
    }

        vTaskDelay(getMonitoringInterval());
    }
}

/* ---------------------------------------------------------
 * Emergency Alert Task
 * Handles critical patient conditions.
 * --------------------------------------------------------- */

static void EmergencyAlertTask(void *pvParameters)
{
    (void) pvParameters;

    /*
     * Emergency alerts are generated only when the patient ENTERS
     * the critical state. This prevents repeated SQLite writes every
     * second while the patient remains critical.
     *
     * The actual database write is performed here only once per
     * critical episode.
     */
    while (1)
    {
        SeverityLevel severitySnapshot = currentSeverity;

        if (severitySnapshot == SEVERITY_CRITICAL &&
            lastAlertSeverity != SEVERITY_CRITICAL)
        {
            printf("\n");
            printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
            printf("!!!       EMERGENCY ALERT             !!!\n");
            printf("!!!       PATIENT CRITICAL            !!!\n");
            printf("!!!   IMMEDIATE ATTENTION REQUIRED    !!!\n");
            printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

            if (!database_insert_alert(
                    SEVERITY_CRITICAL,
                    "Patient condition is critical - immediate attention required"))
            {
                printf("[Database] Failed to store emergency alert.\n");
            }

            lastAlertSeverity = SEVERITY_CRITICAL;
        }
        else if (severitySnapshot != SEVERITY_CRITICAL)
        {
            /* Re-arm the alert for the next critical episode. */
            lastAlertSeverity = severitySnapshot;
        }

        /* Emergency task is only an event checker; it does not poll SQLite. */
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

/* ---------------------------------------------------------
 * Main
 * --------------------------------------------------------- */

int main(void)
{
    printf("\n");
    printf("=============================================\n");
    printf(" Predictive Severity-Aware RTOS\n");
    printf(" Patient Monitoring System\n");
    printf("=============================================\n\n");

        /* Initialize SQLite database */
    if (!database_init())
    {
        printf("[Database] Initialization failed.\n");
        return 1;
    }

    srand((unsigned int)time(NULL));

    /* Start the automatic patient-scenario simulation clock. */
    simulationStartTick = xTaskGetTickCount();

    /* Create mutex */
    vitalsMutex = xSemaphoreCreateMutex();

    if (vitalsMutex == NULL)
    {
        printf("ERROR: Failed to create vitals mutex.\n");
        return 1;
    }

    /* Create unified monitoring task */
    xTaskCreate(
        MonitorTask,
        "Monitor",
        256,
        NULL,
        3,
        &monitorTaskHandle
    );

    /* Create Severity Task */
    xTaskCreate(
        SeverityTask,
        "Severity",
        256,
        NULL,
        4,
        &severityTaskHandle
    );

    xTaskCreate(
        EmergencyAlertTask,
        "Emergency",
        256,
        NULL,
        7,
        &emergencyTaskHandle
    );

    printf("Starting FreeRTOS scheduler...\n\n");

    /* Start the RTOS scheduler */
    vTaskStartScheduler();

    /*
     * The scheduler should never return.
     */
    printf("ERROR: Scheduler stopped!\n");

    return 1;
}

/* ---------------------------------------------------------
 * FreeRTOS application hooks
 * --------------------------------------------------------- */

void vApplicationStackOverflowHook(TaskHandle_t xTask,
                                   char *pcTaskName)
{
    (void)xTask;

    printf("\nERROR: Stack overflow in task: %s\n",
           pcTaskName);

    while (1)
    {
        /* Stop execution */
    }
}


void vApplicationMallocFailedHook(void)
{
    printf("\nERROR: FreeRTOS memory allocation failed!\n");

    while (1)
    {
        /* Stop execution */
    }
}