#ifndef PREDICTION_H
#define PREDICTION_H

#include "../include/vitals.h"

/*
 * Patient deterioration risk levels.
 */
typedef enum
{
    RISK_LOW = 0,
    RISK_MODERATE,
    RISK_HIGH,
    RISK_CRITICAL
} RiskLevel;

/*
 * Calculate deterioration risk from current vital signs.
 */
RiskLevel predictDeterioration(const VitalSigns *vitals);

/*
 * Convert risk level to readable text.
 */
const char *riskLevelToString(RiskLevel risk);

#endif