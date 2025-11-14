const socket = io();

// --- HANDLE BOARD STATUS ---
socket.on("boardStatus", data => {
    console.log("BOARD STATUS:", data);

    const led = document.getElementById("boardLed");
    const statusTxt = document.getElementById("boardStatusTxt");
    const lastSleep = document.getElementById("lastSleep");

    if (data.status === "awake") {
        led.style.background = "green";
        statusTxt.innerText = "Awake (Control window active)";
    }

    if (data.status === "sleep") {
        led.style.background = "red";
        statusTxt.innerText = "Sleeping";

        if (data.time) {
            lastSleep.innerText = data.time;
        }
    }
});

// --- HANDLE MOISTURE DATA ---
socket.on("update", data => {
    console.log("MOISTURE:", data);

    document.getElementById("m1").innerText = data.zones.zone1;
    document.getElementById("m2").innerText = data.zones.zone2;
    document.getElementById("m3").innerText = data.zones.zone3;
    document.getElementById("m4").innerText = data.zones.zone4;
});

// --- SEND PUMP COMMAND ---
function pump(zone, action) {
    socket.emit("pumpCommand", { zone, action });
}
