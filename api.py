from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware


app = FastAPI()


app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)


patient_data = {

    "status":"Disconnected",
    "heartRate":0,
    "bloodPressure":"0/0",
    "spo2":0
}



@app.post("/update")
def update(data:dict):

    print("DATA RECEIVED:")
    print(data)

    patient_data.update(data)

    return {
        "status":"updated",
        "data":patient_data
    }



@app.get("/patient")
def patient():

    return patient_data
