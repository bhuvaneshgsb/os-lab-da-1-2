#include <stdio.h>
#include <sqlite3.h>
#include <time.h>

#include "database.h"

static sqlite3 *db = NULL;


/* Get current timestamp */
static void get_timestamp(char *buffer, size_t size)
{
    time_t now = time(NULL);
    struct tm *time_info = localtime(&now);

    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", time_info);
}


/* Convert severity enum to text */
static const char *severity_to_string(SeverityLevel severity)
{
    switch (severity)
    {
        case SEVERITY_NORMAL:
            return "NORMAL";

        case SEVERITY_LOW:
            return "LOW";

        case SEVERITY_MODERATE:
            return "MODERATE";

        case SEVERITY_HIGH:
            return "HIGH";

        case SEVERITY_CRITICAL:
            return "CRITICAL";

        default:
            return "UNKNOWN";
    }
}


/* Initialize SQLite database */
int database_init(void)
{
    int result;

    result = sqlite3_open("database/patient_monitoring.db", &db);

    if (result != SQLITE_OK)
    {
        printf("[Database] Failed to open database: %s\n",
               sqlite3_errmsg(db));

        return 0;
    }

    const char *sql =
        "CREATE TABLE IF NOT EXISTS vital_readings ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "timestamp TEXT NOT NULL,"
        "heart_rate INTEGER NOT NULL,"
        "spo2 INTEGER NOT NULL,"
        "temperature REAL NOT NULL"
        ");"

        "CREATE TABLE IF NOT EXISTS severity_history ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "timestamp TEXT NOT NULL,"
        "heart_rate INTEGER NOT NULL,"
        "spo2 INTEGER NOT NULL,"
        "temperature REAL NOT NULL,"
        "severity TEXT NOT NULL"
        ");"

        "CREATE TABLE IF NOT EXISTS emergency_alerts ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "timestamp TEXT NOT NULL,"
        "severity TEXT NOT NULL,"
        "message TEXT NOT NULL"
        ");"

        "CREATE TABLE IF NOT EXISTS scheduler_history ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "timestamp TEXT NOT NULL,"
        "severity TEXT NOT NULL,"
        "priority INTEGER NOT NULL,"
        "sampling_interval_ms INTEGER NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS prediction_history ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "timestamp TEXT NOT NULL,"
        "heart_rate INTEGER NOT NULL,"
        "spo2 INTEGER NOT NULL,"
        "temperature REAL NOT NULL,"
        "risk_level TEXT NOT NULL"
        ");";
    char *error_message = NULL;

    result = sqlite3_exec(
        db,
        sql,
        NULL,
        NULL,
        &error_message
    );

    if (result != SQLITE_OK)
    {
        printf("[Database] Table creation failed: %s\n",
               error_message);

        sqlite3_free(error_message);
        sqlite3_close(db);
        db = NULL;

        return 0;
    }

    printf("[Database] SQLite database initialized successfully.\n");

    return 1;
}


/* Insert vital signs */
int database_insert_vitals(const VitalSigns *vitals)
{
    if (db == NULL || vitals == NULL)
        return 0;

    char timestamp[32];
    char sql[512];

    get_timestamp(timestamp, sizeof(timestamp));

    snprintf(
        sql,
        sizeof(sql),
        "INSERT INTO vital_readings "
        "(timestamp, heart_rate, spo2, temperature) "
        "VALUES ('%s', %u, %u, %.1f);",
        timestamp,
        vitals->heart_rate,
        vitals->spo2,
        vitals->temperature
    );

    if (sqlite3_exec(db, sql, NULL, NULL, NULL) != SQLITE_OK)
        return 0;

    return 1;
}


/* Insert severity information */
int database_insert_severity(
    const VitalSigns *vitals,
    SeverityLevel severity)
{
    if (db == NULL || vitals == NULL)
        return 0;

    char timestamp[32];
    char sql[512];

    get_timestamp(timestamp, sizeof(timestamp));

    snprintf(
        sql,
        sizeof(sql),
        "INSERT INTO severity_history "
        "(timestamp, heart_rate, spo2, temperature, severity) "
        "VALUES ('%s', %u, %u, %.1f, '%s');",
        timestamp,
        vitals->heart_rate,
        vitals->spo2,
        vitals->temperature,
        severity_to_string(severity)
    );

    if (sqlite3_exec(db, sql, NULL, NULL, NULL) != SQLITE_OK)
        return 0;

    return 1;
}


/* Insert emergency alert */
int database_insert_alert(
    SeverityLevel severity,
    const char *message)
{
    if (db == NULL || message == NULL)
        return 0;

    char timestamp[32];
    char sql[512];

    get_timestamp(timestamp, sizeof(timestamp));

    snprintf(
        sql,
        sizeof(sql),
        "INSERT INTO emergency_alerts "
        "(timestamp, severity, message) "
        "VALUES ('%s', '%s', '%s');",
        timestamp,
        severity_to_string(severity),
        message
    );

    if (sqlite3_exec(db, sql, NULL, NULL, NULL) != SQLITE_OK)
        return 0;

    return 1;
}


/* Insert scheduler information */
int database_insert_scheduler(
    SeverityLevel severity,
    int priority,
    int sampling_interval_ms)
{
    if (db == NULL)
        return 0;

    char timestamp[32];
    char sql[512];

    get_timestamp(timestamp, sizeof(timestamp));

    snprintf(
        sql,
        sizeof(sql),
        "INSERT INTO scheduler_history "
        "(timestamp, severity, priority, sampling_interval_ms) "
        "VALUES ('%s', '%s', %d, %d);",
        timestamp,
        severity_to_string(severity),
        priority,
        sampling_interval_ms
    );

    if (sqlite3_exec(db, sql, NULL, NULL, NULL) != SQLITE_OK)
        return 0;

    return 1;
}

/* Insert prediction information */
int database_insert_prediction(
    const VitalSigns *vitals,
    RiskLevel risk)
{
    if (db == NULL || vitals == NULL)
        return 0;

    char timestamp[32];
    char sql[512];

    get_timestamp(timestamp, sizeof(timestamp));

    snprintf(
        sql,
        sizeof(sql),
        "INSERT INTO prediction_history "
        "(timestamp, heart_rate, spo2, temperature, risk_level) "
        "VALUES ('%s', %u, %u, %.1f, '%s');",
        timestamp,
        vitals->heart_rate,
        vitals->spo2,
        vitals->temperature,
        riskLevelToString(risk)
    );

    if (sqlite3_exec(db, sql, NULL, NULL, NULL) != SQLITE_OK)
        return 0;

    return 1;
}
/* Close database */
void database_close(void)
{
    if (db != NULL)
    {
        sqlite3_close(db);
        db = NULL;

        printf("[Database] Database closed.\n");
    }
}