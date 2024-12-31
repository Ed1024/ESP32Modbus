#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <WebServer.h>
#include <ModbusIP_ESP8266.h>

// WiFi Credentials
const char* ssid = "Bernard78";
const char* password = "Excitedspider512! ";

// Modbus and HTTP server
ModbusIP modbus; // Modbus TCP client
WebServer server(80);

// ADAM-6060 Modbus settings
IPAddress adamIP(192, 168, 1, 100); // Replace with ADAM-6060's IP address
const uint16_t coilStartAddress = 0; // Starting coil address
const uint8_t numOutputs = 6;        // Number of outputs (ADAM-6060 supports 6)

// Track output states
bool outputStates[numOutputs] = {false, false, false, false, false, false};
bool outputSuccess[numOutputs] = {false, false, false, false, false, false}; // Track success state

// Function to update output states on ADAM-6060
void updateOutputs() {
  for (uint8_t i = 0; i < numOutputs; i++) {
    uint16_t coilAddress = coilStartAddress + i; // Modbus address of the coil
    bool coilState = outputStates[i];           // Current state of the coil

    // Write the coil value using the Modbus library
    outputSuccess[i] = modbus.writeCoil(adamIP, coilAddress, coilState);
    if (outputSuccess[i]) {
      Serial.println("Successfully updated coil " + String(coilAddress) + " to " + String(coilState));
    } else {
      Serial.println("Failed to write coil " + String(coilAddress));
    }
  }
}

// Web page handler
void handleRoot() {
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
      <title>ADAM-6060 Control</title>
      <style>
        body { font-family: Arial, sans-serif; text-align: center; }
        .button {
          display: inline-block;
          width: 100px;
          height: 50px;
          margin: 10px;
          line-height: 50px;
          text-align: center;
          font-size: 16px;
          color: white;
          background-color: #4CAF50;
          border: none;
          border-radius: 5px;
          cursor: pointer;
        }
        .button.failed {
          background-color: #f44336;
        }
        .button.inactive {
          background-color: #808080;
        }
        .button:active {
          background-color: #3e8e41;
        }
      </style>
      <script>
        function toggleOutput(output) {
          var xhr = new XMLHttpRequest();
          xhr.open("GET", "/toggle?output=" + output, true);
          xhr.onreadystatechange = function() {
            if (xhr.readyState == 4 && xhr.status == 200) {
              location.reload();
            }
          };
          xhr.send();
        }
      </script>
    </head>
    <body>
      <h1>ADAM-6060 Control</h1>
      <div>
  )rawliteral";

  for (uint8_t i = 0; i < numOutputs; i++) {
    html += "<button onclick=\"toggleOutput(" + String(i) + ")\" class=\"button";
    if (!outputSuccess[i]) {
      html += " failed";
    } else if (!outputStates[i]) {
      html += " inactive";
    }
    html += "\">Relay " + String(i + 1) + "</button>";
  }

  html += R"rawliteral(
      </div>
    </body>
    </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

// Handle toggle requests
void handleToggle() {
  if (server.hasArg("output")) {
    int output = server.arg("output").toInt();
    if (output >= 0 && output < numOutputs) {
      outputStates[output] = !outputStates[output];
      updateOutputs();
    }
  }
  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected!");
  Serial.println("ESP32 IP: " + WiFi.localIP().toString());

  // Start Modbus TCP
  modbus.server();              // Initialize Modbus server

  // Setup HTTP server
  server.on("/", handleRoot);
  server.on("/toggle", handleToggle);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  modbus.task(); // Required for Modbus server operations
}