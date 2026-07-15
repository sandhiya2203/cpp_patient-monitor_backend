from fastapi import FastAPI
from fastapi.responses import FileResponse
import json
import os

app = FastAPI()


JSON_FILE = "/home/sandiya/Downloads/sdclib-master/build/bin/patient_data1.json"


@app.get("/")
def home():
    return FileResponse("static/index.html")


@app.get("/patient")
def patient():

    if not os.path.exists(JSON_FILE):
        return {
            "status": "JSON FILE NOT FOUND"
        }

    with open(JSON_FILE, "r") as f:
        data = json.load(f)

    print("Sending:", data)

    return data
