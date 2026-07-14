from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
import json
import os

app = FastAPI()


app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

import os

json_path = os.path.join(
    os.path.dirname(__file__),
    "patient_data.json"
)


@app.get("/")
def home():
    return {
        "message":"SDC Patient REST API Running"
    }


@app.get("/patient")
def patient():

    print("Looking for:", JSON_FILE)


    if not os.path.exists(JSON_FILE):

        print("JSON NOT FOUND")

        return {
            "status":"Disconnected",
            "heart_rate":0,
            "blood_pressure":"0/0",
            "spo2":"0"
        }


    print("JSON FOUND")


    with open(JSON_FILE,"r") as f:

        data = json.load(f)

    return data
