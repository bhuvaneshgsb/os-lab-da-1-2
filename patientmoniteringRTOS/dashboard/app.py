from flask import Flask, render_template, jsonify
import sqlite3
import os

app = Flask(__name__)

DATABASE = os.path.join(
    os.path.dirname(os.path.dirname(__file__)),
    "database",
    "patient_monitoring.db"
)


def get_db_connection():
    conn = sqlite3.connect(DATABASE)
    conn.row_factory = sqlite3.Row
    return conn


# =========================================================
# DASHBOARD
# =========================================================

@app.route("/")
def dashboard():
    return render_template("index.html")


# =========================================================
# LATEST DATA
# =========================================================

@app.route("/api/latest")
def latest_data():

    conn = get_db_connection()

    # -----------------------------------------------------
    # Use the latest severity record as the authoritative
    # patient sample for the current dashboard status.
    # -----------------------------------------------------
    severity = conn.execute(
        """
        SELECT *
        FROM severity_history
        ORDER BY id DESC
        LIMIT 1
        """
    ).fetchone()

    # -----------------------------------------------------
    # Latest prediction
    # -----------------------------------------------------
    prediction = conn.execute(
        """
        SELECT *
        FROM prediction_history
        ORDER BY id DESC
        LIMIT 1
        """
    ).fetchone()

    # -----------------------------------------------------
    # Latest scheduler state
    # -----------------------------------------------------
    scheduler = conn.execute(
        """
        SELECT *
        FROM scheduler_history
        ORDER BY id DESC
        LIMIT 1
        """
    ).fetchone()

    # -----------------------------------------------------
    # Build vitals from the SAME severity record.
    # This prevents:
    #
    # HR/SpO2/Temperature = one sample
    # Severity             = another sample
    #
    # -----------------------------------------------------
    if severity:
        vitals = {
            "timestamp": severity["timestamp"],
            "heart_rate": severity["heart_rate"],
            "spo2": severity["spo2"],
            "temperature": severity["temperature"]
        }
    else:
        vitals = {}

    conn.close()

    return jsonify({

        "vitals": vitals,

        "severity":
            dict(severity) if severity else {},

        "prediction":
            dict(prediction) if prediction else {},

        "scheduler":
            dict(scheduler) if scheduler else {}

    })


# =========================================================
# VITAL HISTORY
# =========================================================

@app.route("/api/history")
def history():

    conn = get_db_connection()


    rows = conn.execute(
        """
        SELECT
            timestamp,
            heart_rate,
            spo2,
            temperature
        FROM vital_readings
        ORDER BY id DESC
        LIMIT 20
        """
    ).fetchall()


    conn.close()


    return jsonify(
        [dict(row) for row in rows]
    )


# =========================================================
# EMERGENCY ALERTS
# =========================================================

@app.route("/api/alerts")
def alerts():

    conn = get_db_connection()


    rows = conn.execute(
        """
        SELECT
            timestamp,
            severity,
            message
        FROM emergency_alerts
        ORDER BY id DESC
        LIMIT 10
        """
    ).fetchall()


    conn.close()


    return jsonify(
        [dict(row) for row in rows]
    )


# =========================================================
# PREDICTION HISTORY
# =========================================================

@app.route("/api/predictions")
def predictions():

    conn = get_db_connection()


    rows = conn.execute(
        """
        SELECT *
        FROM prediction_history
        ORDER BY id DESC
        LIMIT 20
        """
    ).fetchall()


    conn.close()


    return jsonify(
        [dict(row) for row in rows]
    )


# =========================================================
# START SERVER
# =========================================================

if __name__ == "__main__":

    print("==============================================")
    print(" Predictive Severity-Aware RTOS Dashboard")
    print("==============================================")

    print("Database:", DATABASE)

    print("Starting dashboard server...")


    app.run(
        host="127.0.0.1",
        port=5000,
        debug=False
    )