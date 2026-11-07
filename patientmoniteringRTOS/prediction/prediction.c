#include "prediction.h"

/*
 * Rule-based deterioration prediction for the RTOS simulation.
 *
 * The thresholds are intentionally separated from the severity model
 * so that moderate abnormalities do not automatically become CRITICAL.
 *
 * This is a software simulation, not a clinical diagnostic model.
 */
RiskLevel predictDeterioration(const VitalSigns *vitals)
{
    int riskScore = 0;

    if (vitals == NULL)
    {
        return RISK_CRITICAL;
    }

    /*
     * ----------------------------------------------------
     * 1. HEART-RATE RISK
     * ----------------------------------------------------
     *
     * Mild:     105-119
     * High:     120-134
     * Critical: >=135
     */
    if (vitals->heart_rate >= 135 ||
        vitals->heart_rate <= 45)
    {
        riskScore += 4;
    }
    else if (vitals->heart_rate >= 120 ||
             vitals->heart_rate <= 50)
    {
        riskScore += 3;
    }
    else if (vitals->heart_rate >= 105 ||
             vitals->heart_rate <= 55)
    {
        riskScore += 1;
    }

    /*
     * ----------------------------------------------------
     * 2. SpO2 RISK
     * ----------------------------------------------------
     *
     * Mild:     95-96
     * Moderate: 91-94
     * High:     86-90
     * Critical: <=85
     */
    if (vitals->spo2 <= 85)
    {
        riskScore += 4;
    }
    else if (vitals->spo2 <= 90)
    {
        riskScore += 3;
    }
    else if (vitals->spo2 <= 94)
    {
        riskScore += 2;
    }
    else if (vitals->spo2 <= 96)
    {
        riskScore += 1;
    }

    /*
     * ----------------------------------------------------
     * 3. TEMPERATURE RISK
     * ----------------------------------------------------
     *
     * Mild:     >=37.8
     * Moderate: >=38.0
     * High:     >=39.0
     * Critical: >=40.0
     */
    if (vitals->temperature >= 40.0f ||
        vitals->temperature <= 35.0f)
    {
        riskScore += 4;
    }
    else if (vitals->temperature >= 39.0f ||
             vitals->temperature <= 35.5f)
    {
        riskScore += 3;
    }
    else if (vitals->temperature >= 38.0f)
    {
        riskScore += 2;
    }
    else if (vitals->temperature >= 37.8f)
    {
        riskScore += 1;
    }

    /*
     * ----------------------------------------------------
     * 4. FINAL RISK LEVEL
     * ----------------------------------------------------
     *
     * >=10 : CRITICAL
     * >=7  : HIGH
     * >=4  : MODERATE
     * <4   : LOW
     *
     * Example:
     * HR=109, SpO2=91, Temp=38.0
     *       1 + 2 + 2 = 5
     *       -> MODERATE RISK
     *
     * Example:
     * HR=141, SpO2=83, Temp=40.1
     *       4 + 4 + 4 = 12
     *       -> CRITICAL RISK
     */
    if (riskScore >= 10)
    {
        return RISK_CRITICAL;
    }
    else if (riskScore >= 7)
    {
        return RISK_HIGH;
    }
    else if (riskScore >= 4)
    {
        return RISK_MODERATE;
    }

    return RISK_LOW;
}


/*
 * Convert risk level to readable text.
 */
const char *riskLevelToString(RiskLevel risk)
{
    switch (risk)
    {
        case RISK_LOW:
            return "LOW RISK";

        case RISK_MODERATE:
            return "MODERATE RISK";

        case RISK_HIGH:
            return "HIGH RISK";

        case RISK_CRITICAL:
            return "CRITICAL RISK";

        default:
            return "UNKNOWN";
    }
}
