#ifndef WEBPAGE_H
#define WEBPAGE_H

const char* webpage = R"=====(

<!DOCTYPE html>
<html lang="de">
<head>
    <meta charset="UTF-8">
    <title>ESP32 Wetterstation</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css" rel="stylesheet">
</head>
<body class="bg-light">

    <div class="container text-center mt-5">
        <h1 class="mb-4">ESP32 Wetterstation</h1>

        <div class="card mb-4 shadow-sm">
            <div class="card-body">
                <h2 class="card-title">Messwerte</h2>
                <p class="card-text">Temperatur: <span id="temperature" class="text-danger fw-bold">Loading...</span> &#8451;</p>
                <p class="card-text">Luftfeuchtigkeit: <span id="humidity" class="text-danger fw-bold">Loading...</span> %</p>
                <p class="card-text"><span id="time">Loading...</span></p>
            </div>
        </div>

        <div class="card shadow-sm">
            <div class="card-body">
                <h2 class="card-title">LED Steuerung</h2>
                <button class="btn btn-primary mb-2" onclick="toggleLED()">LED Umschalten</button>
                <p class="card-text">Status: <span id="ledStatus" class="fw-bold">Unbekannt</span></p>
            </div>
        </div>
    </div>

    <script>
        function fetchTemperature() {
            fetch("/temperature")
                .then(response => response.text())
                .then(data => {
                    document.getElementById("temperature").textContent = data;
                });
        }

        function fetchHumidity() {
            fetch("/humidity")
                .then(response => response.text())
                .then(data => {
                    if (!isNaN(data)) {
                        document.getElementById("humidity").textContent = data;
                    }
                });
        }

        function fetchTime() {
            fetch("/time")
                .then(response => response.text())
                .then(data => {
                    document.getElementById("time").textContent = data;
                });
        }

        let ledOn = false;

        function toggleLED() {
            ledOn = !ledOn;
            fetch("/led?state=" + (ledOn ? "on" : "off"))
                .then(response => response.text())
                .then(data => {
                    document.getElementById("ledStatus").textContent = data;
                });
        }

        window.onload = () => {
            fetch("/ledStatus")
                .then(response => response.text())
                .then(data => {
                    document.getElementById("ledStatus").textContent = data;
                    ledOn = (data === "LED ist AN");
                });

            fetchTemperature();
            fetchHumidity();
            fetchTime();

            setInterval(fetchTemperature, 4000);
            setInterval(fetchHumidity, 4000);
            setInterval(fetchTime, 4000);
        };
    </script>

</body>
</html>

)=====";

#endif