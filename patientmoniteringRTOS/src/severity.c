#include "severity.h"

/*
 * Previous vital signs used to detect rapid changes.
 */
static VitalSigns previousVitals = {0};

/*
 * Number of consecutive abnormal readings.
 */
static int abnormalCount = 0;

/*
 * Calculate severity using:
 * 1. Current vital values
 * 2. Rate of change
 * 3. Persistence of abnormal values
 */
SeverityLevel calculateSeverity(const VitalSigns *vitals)
{
    int score = 0;

    /*
     * ----------------------------------------------------
     * 1. CURRENT VITAL SIGN SCORE
     * ----------------------------------------------------
     */

    /* Heart Rate */
    if (vitals->heart_rate < 40 ||
        vitals->heart_rate > 130)
    {
        score += 4;
    }
    else if (vitals->heart_rate < 50 ||
             vitals->heart_rate > 110)
    {
        score += 3;
    }
    else if (vitals->heart_rate < 60 ||
             vitals->heart_rate > 100)
    {
        score += 1;
    }

    /* SpO2 */
    if (vitals->spo2 < 90)
    {
        score += 5;
    }
    else if (vitals->spo2 < 94)
    {
        score += 4;
    }
    else if (vitals->spo2 < 96)
    {
        score += 2;
    }
    else if (vitals->spo2 < 97)
    {
        score += 1;
    }

    /* Temperature */
    if (vitals->temperature >= 40.0f ||
        vitals->temperature < 35.0f)
    {
        score += 5;
    }
    else if (vitals->temperature >= 39.0f ||
             vitals->temperature < 35.5f)
    {
        score += 4;
    }
    else if (vitals->temperature >= 38.0f ||
             vitals->temperature < 36.0f)
    {
        score += 2;
    }
    else if (vitals->temperature >= 37.5f)
    {
        score += 1;
    }


    /*
     * ----------------------------------------------------
     * 2. PERSISTENCE DETECTION
     * ----------------------------------------------------
     */

    if (score > 0)
    {
        abnormalCount++;
    }
    else
    {
        abnormalCount = 0;
    }

    /*
     * If abnormal readings continue,
     * increase severity gradually.
     */
    if (abnormalCount >= 3)
    {
        score += 2;
    }

    if (abnormalCount >= 5)
    {
        score += 2;
    }


    /*
     * ----------------------------------------------------
     * 3. RATE-OF-CHANGE DETECTION
     * ----------------------------------------------------
     */

    if (previousVitals.heart_rate != 0)
    {
        int heartRateChange =
            (int)vitals->heart_rate -
            (int)previousVitals.heart_rate;

        if (heartRateChange < 0)
        {
            heartRateChange = -heartRateChange;
        }

        /*
         * Rapid heart-rate change.
         */
        if (heartRateChange >= 25)
        {
            score += 2;
        }
        else if (heartRateChange >= 15)
        {
            score += 1;
        }
    }

    if (previousVitals.spo2 != 0)
    {
        int spo2Change =
            (int)previousVitals.spo2 -
            (int)vitals->spo2;

        /*
         * SpO2 dropping rapidly is important.
         */
        if (spo2Change >= 4)
        {
            score += 3;
        }
        else if (spo2Change >= 2)
        {
            score += 1;
        }
    }


    /*
     * Save current values for the next calculation.
     */
    previousVitals = *vitals;


    /*
     * ----------------------------------------------------
     * 4. FINAL PREDICTIVE SEVERITY
     * ----------------------------------------------------
     */

    if (score >= 9)
    {
        return SEVERITY_CRITICAL;
    }
    else if (score >= 6)
    {
        return SEVERITY_HIGH;
    }
    else if (score >= 3)
    {
        return SEVERITY_MODERATE;
    }
    else if (score >= 1)
    {
        return SEVERITY_LOW;
    }

    return SEVERITY_NORMAL;
}