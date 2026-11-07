import sqlite3
from pathlib import Path
from datetime import datetime


# Database location
DATABASE_PATH = Path(__file__).parent / "patient_monitoring.db"


def get_connection():
    """Create a connection to the SQLite database."""
    return sqlite3.connect(DATABASE_PATH)


def initialize_database():
    """Create all required database tables."""

    connection = get_connection()
    cursor = connection.cursor()

    # Store patient vital-sign readings
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS vital_readings (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp TEXT NOT NULL,
            heart_rate INTEGER NOT NULL,
            spo2 INTEGER NOT NULL,
            temperature REAL NOT NULL
        )
    """)

    # Store severity calculations
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS severity_history (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp TEXT NOT NULL,
            heart_rate INTEGER NOT NULL,
            spo2 INTEGER NOT NULL,
            temperature REAL NOT NULL,
            severity TEXT NOT NULL
        )
    """)

    # Store emergency alerts
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS emergency_alerts (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp TEXT NOT NULL,
            severity TEXT NOT NULL,
            message TEXT NOT NULL
        )
    """)

    # Store RTOS scheduling information
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS scheduler_history (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp TEXT NOT NULL,
            severity TEXT NOT NULL,
            priority INTEGER NOT NULL,
            sampling_interval_ms INTEGER NOT NULL
        )
    """)

    connection.commit()
    connection.close()


def insert_vital_reading(heart_rate, spo2, temperature):
    """Insert a new vital-sign reading."""

    connection = get_connection()
    cursor = connection.cursor()

    timestamp = datetime.now().isoformat(timespec="seconds")

    cursor.execute("""
        INSERT INTO vital_readings
        (timestamp, heart_rate, spo2, temperature)
        VALUES (?, ?, ?, ?)
    """, (timestamp, heart_rate, spo2, temperature))

    connection.commit()
    connection.close()


def insert_severity(
    heart_rate,
    spo2,
    temperature,
    severity
):
    """Store a severity calculation."""

    connection = get_connection()
    cursor = connection.cursor()

    timestamp = datetime.now().isoformat(timespec="seconds")

    cursor.execute("""
        INSERT INTO severity_history
        (timestamp, heart_rate, spo2, temperature, severity)
        VALUES (?, ?, ?, ?, ?)
    """, (
        timestamp,
        heart_rate,
        spo2,
        temperature,
        severity
    ))

    connection.commit()
    connection.close()


def insert_emergency_alert(severity, message):
    """Store an emergency alert."""

    connection = get_connection()
    cursor = connection.cursor()

    timestamp = datetime.now().isoformat(timespec="seconds")

    cursor.execute("""
        INSERT INTO emergency_alerts
        (timestamp, severity, message)
        VALUES (?, ?, ?)
    """, (
        timestamp,
        severity,
        message
    ))

    connection.commit()
    connection.close()


def insert_scheduler_event(
    severity,
    priority,
    sampling_interval_ms
):
    """Store RTOS scheduling information."""

    connection = get_connection()
    cursor = connection.cursor()

    timestamp = datetime.now().isoformat(timespec="seconds")

    cursor.execute("""
        INSERT INTO scheduler_history
        (timestamp, severity, priority, sampling_interval_ms)
        VALUES (?, ?, ?, ?)
    """, (
        timestamp,
        severity,
        priority,
        sampling_interval_ms
    ))

    connection.commit()
    connection.close()


if __name__ == "__main__":
    initialize_database()
    print("Patient Monitoring database initialized successfully.")
    print(f"Database location: {DATABASE_PATH}")