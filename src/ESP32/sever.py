from flask import Flask, request, jsonify
import joblib
import numpy as np
from datetime import datetime, timedelta
import os


app = Flask(__name__)


MODEL_PH_PATH = "model_PH.pkl"
MODEL_KPH_PATH = "model_KPH.pkl"


if not os.path.exists(MODEL_PH_PATH) or not os.path.exists(MODEL_KPH_PATH):

    raise FileNotFoundError("Không tìm thấy model_PH.pkl hoặc model_KPH.pkl trong folder hiện tại.")


model_PH = joblib.load(MODEL_PH_PATH)
model_KPH = joblib.load(MODEL_KPH_PATH)


def get_prediction_time_and_day(hours_float):

    if hours_float is None:
        return None, None
    try:

        if hours_float < 0:
            hours_float = 0.0


        minutes_total = int(round(hours_float * 60))
        now = datetime.now()
        full_time = now + timedelta(minutes=minutes_total)

           day_str = "NAY"
        if full_time.day != now.day:
            day_str = "MAI"

        time_str = full_time.strftime("%H:%M")

        return time_str, day_str
    except Exception:

        return None, None


@app.route("/", methods=["GET"])
def home():

    return "Trash-predict API running"


@app.route("/predict", methods=["POST"])
def predict():

    data = request.get_json()


    if not data:

        return jsonify({"error": "Expecting JSON body"}), 400

    try:

        PH_percent = float(data.get("PH_percent", data.get("ph_percent", data.get("ph", 0))))
        KPH_percent = float(data.get("KPH_percent", data.get("kph_percent", data.get("kph", 0))))


        now_dt = datetime.now()
        hour = int(data.get("hour", now_dt.hour))

        day = int(data.get("day", now_dt.weekday()))

        PH_change = float(data.get("PH_change", 0.0))
        KPH_change = float(data.get("KPH_change", 0.0))
    except Exception as e:

        return jsonify({"error": f"Invalid input fields: {e}"}), 400

    X = np.array([[PH_percent, KPH_percent, hour, day, PH_change, KPH_change]])


    ph_hours = float(model_PH.predict(X)[0])
    kph_hours = float(model_KPH.predict(X)[0])

    if ph_hours > 1e4:
        ph_hours = None
    if kph_hours > 1e4:
        kph_hours = None

    ph_time_str, ph_day_str = get_prediction_time_and_day(ph_hours)
    kph_time_str, kph_day_str = get_prediction_time_and_day(kph_hours)

    response = {
        "PH_Time_Hours": None if ph_hours is None else round(ph_hours, 3),
        "PH_Full_At": ph_time_str,
        "PH_Day": ph_day_str,
        "KPH_Time_Hours": None if kph_hours is None else round(kph_hours, 3),
        "KPH_Full_At": kph_time_str,
        "KPH_Day": kph_day_str,
    }


    return jsonify(response), 200


if __name__ == "__main__":

    app.run(host="0.0.0.0", port=5000, debug=True)