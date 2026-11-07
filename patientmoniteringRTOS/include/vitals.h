#ifndef VITALS_H
#define VITALS_H

#include <stdint.h>

/*
 * Patient vital signs.
 *
 * Later, these values can come from real sensors
 * instead of the software simulator.
 */
typedef struct
{
    uint16_t heart_rate;       // BPM
    uint8_t  spo2;             // %
    float    temperature;      // °C

} VitalSigns;

/*
 * Severity levels used by the RTOS.
 */
typedef enum
{
    SEVERITY_NORMAL = 0,
    SEVERITY_LOW,
    SEVERITY_MODERATE,
    SEVERITY_HIGH,
    SEVERITY_CRITICAL

} SeverityLevel;

#endif