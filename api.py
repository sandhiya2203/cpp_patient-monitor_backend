from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from datetime import datetime
from zoneinfo import ZoneInfo
import threading
import time


app = FastAPI()


# -----------------------------
# CORS for Vercel frontend
# -----------------------------

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)



# -----------------------------
# Patient Data
# -----------------------------

patient_data = {

    "status": "Disconnected",

    "heartRate": 0,

    "bloodPressure": "0/0",

    "spo2": 0,

    "date": "",

    "time": ""

}



# Last received data time

last_update_time = 0


# Disconnect timeout

TIMEOUT = 10



# -----------------------------
# Background connection checker
# -----------------------------

def check_connection():

    global last_update_time


    while True:


        if last_update_time != 0:


            elapsed = time.time() - last_update_time


            print(
                "Seconds since last data:",
                int(elapsed)
            )



            if elapsed > TIMEOUT:


                if patient_data["status"] != "Disconnected":


                    patient_data["status"] = "Disconnected"


                    print(
                        "PROVIDER DISCONNECTED"
                    )



        time.sleep(2)





# -----------------------------
# Start background thread
# -----------------------------

@app.on_event("startup")
def startup_event():


    thread = threading.Thread(

        target=check_connection,

        daemon=True

    )


    thread.start()






# -----------------------------
# Receive data from C++
# -----------------------------

@app.post("/update")
def update(data: dict):


    global last_update_time



    print("DATA FROM C++")

    print(data)




    # Provider alive

    patient_data["status"] = "Connected"



    # Update values


    patient_data["heartRate"] = data.get(

        "heartRate",

        patient_data["heartRate"]

    )



    patient_data["bloodPressure"] = data.get(

        "bloodPressure",

        patient_data["bloodPressure"]

    )



    patient_data["spo2"] = data.get(

        "spo2",

        patient_data["spo2"]

    )





    # Update timestamp only when data arrives


 now = datetime.now(ZoneInfo("Asia/Kolkata"))



    patient_data["date"] = now.strftime(

        "%d-%m-%Y"

    )



    patient_data["time"] = now.strftime(

        "%H:%M:%S"

    )





    # Store last received time


    last_update_time = time.time()





    return {


        "status": "updated",


        "data": patient_data


    }





# -----------------------------
# Frontend reads this
# -----------------------------

@app.get("/patient")
def patient():


    return patient_data
