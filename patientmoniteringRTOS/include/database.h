#ifndef DATABASE_H
#define DATABASE_H

#include "vitals.h"
#include "severity.h"
#include "../prediction/prediction.h"
/* Initialize the SQLite database */
int database_init(void);

/* Store patient vital readings */
int database_insert_vitals(const VitalSigns *vitals);

/* Store severity information */
int database_insert_severity(
    const VitalSigns *vitals,
    SeverityLevel severity
);

/* Store an emergency alert */
int database_insert_alert(
    SeverityLevel severity,
    const char *message
);

/* Store RTOS scheduler information */
int database_insert_scheduler(
    SeverityLevel severity,
    int priority,
    int sampling_interval_ms
);
int database_insert_prediction(
    const VitalSigns *vitals,
    RiskLevel risk);
/* Close the database */
void database_close(void);

#endif