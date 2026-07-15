from fastapi import FastAPI
from fastapi.staticfiles import StaticFiles
from fastapi.responses import FileResponse
from fastapi.middleware.cors import CORSMiddleware


app = FastAPI()


app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

patient_data = {

    "status": "Disconnected",
    "heartRate": 0,
    "bloodPressure": "0/0",
    "spo2": 0

}



@app.post("/update")
def update(data: dict):

    print("DATA RECEIVED FROM C++:")
    print(data)

    patient_data["status"] = data.get(
        "status",
        patient_data["status"]
    )

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


    return {
        "message":"updated",
        "data":patient_data
    }



@app.get("/patient")
def patient():

    return patient_data



@app.get("/")
def home():

    return FileResponse(
        "static/index.html"
    )



app.mount(
    "/static",
    StaticFiles(directory="static"),
    name="static"
)
