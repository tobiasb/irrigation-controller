#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// Set your Static IP address
IPAddress local_IP(192, 168, 1, 162);
IPAddress gateway(192, 168, 1, 254);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);

const char* ssid = "<your wifi ssid>";
const char* password = "<your wifi password>";

const char* secret_token = "<some symmetric key to offer basic security>";

ESP8266WebServer server(80);

uint8_t relayPin = D1;

void setup() {
  Serial.begin(115200);
  digitalWrite(relayPin, HIGH);
  pinMode(relayPin, OUTPUT);

  WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);
  WiFi.begin(ssid, password);
  delay(100);

  Serial.print("Connecting to ");
  Serial.print(ssid); Serial.println(" ...");

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(1000);
    Serial.print(++i); Serial.print(' ');
  }

  Serial.println('\n');
  Serial.println("Connection established!");
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());

  server.on("/trigger", handle_trigger);
  server.onNotFound(handle_NotFound);

  server.begin();
  Serial.println("HTTP server started");
}

bool triggerStatus = false;
unsigned long triggerDurationSeconds = 0;
unsigned long lastTriggerStart = 0;

void loop() 
{
  server.handleClient();

  if (triggerStatus)
  {
    if (lastTriggerStart == 0)
    {
      lastTriggerStart = millis();
      Serial.println("ON");
      digitalWrite(relayPin, LOW);
    }
    else if (millis() > lastTriggerStart + (triggerDurationSeconds * 1000))
    {
      Serial.println("OFF");
      triggerStatus = false;
      lastTriggerStart = 0;
      digitalWrite(relayPin, HIGH);
    }
  }
}

void handle_trigger() 
{
  if (triggerStatus)
  {
    server.send(400, "text/plain", "Already triggered");
    return;
  }

  Serial.println("Triggered!");
  if (server.arg("token") != secret_token)
  {
    Serial.println("Unauthenticated!");
    server.send(404, "text/plain", "Not found"); //Don't reveal anything if not authenticated
    return;
  }

  if (server.arg("seconds") == NULL)
  {
    server.send(400, "text/plain", "seconds missing");
    return;
  }
  triggerDurationSeconds = server.arg("seconds").toInt();

  if (triggerDurationSeconds <= 0)
  {
    server.send(400, "text/plain", "Invalid duration");
    return;
  }

  triggerStatus = true;
 
  server.send(200, "text/plain", "");
}

void handle_NotFound() {
  server.send(404, "text/plain", "Not found");
}
