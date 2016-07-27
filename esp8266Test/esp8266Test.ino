ADC_MODE(ADC_VCC);

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

const char* ssid = "ssid";
const char* password = "password";
MDNSResponder mdns;

ESP8266WebServer server(80);

const int led = 13;

void handleRoot() {
    digitalWrite(led, 1);
    server.send(200, "text/plain", "hello from esp8266!");
    digitalWrite(led, 0);
    Serial.println("GET root v2");
}

void handleInfo(){
    String message = "esp8266 info:\n";

    int id = ESP.getFlashChipId();
    message += "Flash ID: " + String(id, HEX) + "\n";
    int size = ESP.getFlashChipSize();
    message += "Flash size: " + String(size / 1024) + "Kb\n";
    int speed = ESP.getFlashChipSpeed();
    message += "Mem speed: " + String((double)speed / 1000000.0) + "MHz\n";
    int cpu = ESP.getCpuFreqMHz();
    message += "CPU: " + String(cpu) + "MHz\n";
    int vcc = ESP.getVcc();
    message += "VCC: " + String((double)vcc / 1000.0) + "V\n";

    Serial.println(message);

    server.send(200, "text/plain", message);
}

void handleNotFound(){
    digitalWrite(led, 1);
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i = 0; i < server.args(); i++){
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
    digitalWrite(led, 0);
    Serial.println("GET");
    Serial.println(server.uri());
    Serial.println("not found");
}

void setup(void){
    pinMode(led, OUTPUT);
    digitalWrite(led, 0);
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    Serial.println("");

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    if (mdns.begin("esp8266", WiFi.localIP())) {
        Serial.println("MDNS responder started");
    }

    server.on("/", handleRoot);

    server.on("/inline", [](){
        server.send(200, "text/plain", "this works as well");
    });
    server.on("/info", handleInfo);
    server.on("/reset", [](){
        ESP.restart();
    });

    server.onNotFound(handleNotFound);

    server.begin();
    Serial.println("HTTP server started");
}

void loop(void){
    server.handleClient();
}
