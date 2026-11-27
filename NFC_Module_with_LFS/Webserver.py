from fastapi import FastAPI
from pydantic import BaseModel
from typing import List

app = FastAPI()


class PunchCard(BaseModel):
    recordId: str
    cardIdentifier: str
    punchOn: str
    latitude: float
    longitude: float


class AttendancePayload(BaseModel):
    deviceIdentifier: str
    punchCards: List[PunchCard]


@app.post("/api/attendance")
async def receive_attendance(data: AttendancePayload):

    print("Device:", data.deviceIdentifier)

    for punch in data.punchCards:
        print("Record ID:", punch.recordId)
        print("Card UID:", punch.cardIdentifier)
        print("Punch Time:", punch.punchOn)
        print("Latitude:", punch.latitude)
        print("Longitude:", punch.longitude)

    return {
        "success": True,
        "message": "Attendance received successfully",
        "total": len(data.punchCards)
    }