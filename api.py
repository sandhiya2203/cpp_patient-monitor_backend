from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from datetime import datetime
import threading
import time
import os


app = FastAPI()

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)
LAST_UPDATE_FILE = "last_update.txt"


patient_data = {

    "status": "Disconnected",

    "heartRate": 0,

    "bloodPressure": "0/0",

    "spo2": 0,

    "date": "",

    "time": ""

}


TIMEOUT = 10



def check_connection():

    while True:

        try:

            if os.path.exists(LAST_UPDATE_FILE):

                with open(LAST_UPDATE_FILE, "r") as f:

                    last = float(f.read())


                if time.time() - last > TIMEOUT:

                    patient_data["status"] = "Disconnected"

                    print(
                        "NO DATA - DISCONNECTED"
                    )


        except Exception as e:

            print(e)


        time.sleep(2)





@app.on_event("startup")
def startup_event():

    thread = threading.Thread(
        target=check_connection,
        daemon=True
    )

    thread.start()





@app.post("/update")
def update(data: dict):


    print("DATA FROM C++")

    print(data)



    received_status = data.get(
        "status",
        "Connected"
    )


    # Every received metric means connected

    if received_status == "Connected":


        patient_data["status"] = "Connected"



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



        now = datetime.now()


        patient_data["date"] = now.strftime(
            "%d-%m-%Y"
        )


        patient_data["time"] = now.strftime(
            "%H:%M:%S"
        )



        # save last received time

        with open(LAST_UPDATE_FILE,"w") as f:

            f.write(
                str(time.time())
            )




    return {

        "status":"updated",

        "data":patient_data

    }





@app.get("/patient")
def patient():

    return patient_data










