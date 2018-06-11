#include <ESP8266WiFi.h>
#include <WiFiUDP.h>

// WiFi variables
const char* ssid = "tick"; // ssid
const char* password = "boomboom";// password
boolean wifiConnected = false;
IPAddress ip(10, 0, 0, 101);
IPAddress gateway(10, 0, 0, 1);
IPAddress subnet(255, 255, 255, 0);

// UDP variables
unsigned int localPort = 5280;
WiFiUDP UDP;
boolean udpConnected = false;
char packetBuffer[UDP_TX_PACKET_MAX_SIZE]; //buffer to hold incoming packet,
char ReplyBuffer[] = "acknowledged"; // a string to send back

// drum machine Variables
const int pins[] = {16, 5, 4, 0}; // labeled D0, D1, D2, D3, D4 on nodeMCU PCB
const int nNotes = 4;
const int nBeats = 16;
// how long to pulse in ms
unsigned int onTime = 10;
// keep track of the last time a motor channel (note) was triggered
unsigned long lastHit[nNotes];
// keep track if a note is currently on (high) or if it has been released
bool noteHigh[nNotes];
int pattern[3][4][16] = {{
    {1000, 0, 0, 0, 1000, 0, 0, 0, 800, 0, 0, 0, 800, 0, 0, 0},
    {0, 1000, 0, 0, 0, 1000, 0, 0, 0, 800, 0, 0, 0, 800,0, 0},
    {0, 0, 1000, 0, 0, 0, 1000, 0, 0, 0, 800, 0, 0, 0, 800,0},
    {0, 0, 0, 1000, 0, 0, 0, 1000, 0, 0, 0, 800, 0, 0, 0, 800}
  },
  {
   {1000, 0, 0, 0, 1000, 0, 0, 0, 800, 0, 0, 0, 800, 0, 0, 0},
    {0, 1000, 0, 0, 0, 1000, 0, 0, 0, 800, 0, 0, 0, 800,0, 0},
    {0, 0, 1000, 0, 0, 0, 1000, 0, 0, 0, 800, 0, 0, 0, 800,0},
    {0, 0, 0, 1000, 0, 0, 0, 1000, 0, 0, 0, 800, 0, 0, 0, 800}
  },
  {
    {1000, 0, 0, 0, 1000, 0, 0, 0, 800, 0, 0, 0, 800, 0, 0, 0},
    {0, 1000, 0, 0, 0, 1000, 0, 0, 0, 800, 0, 0, 0, 800,0, 0},
    {0, 0, 1000, 0, 0, 0, 1000, 0, 0, 0, 800, 0, 0, 0, 800,0},
    {0, 0, 0, 1000, 0, 0, 0, 1000, 0, 0, 0, 800, 0, 0, 0, 800}
  }
};

int liklihood[4][16] = {
  {90, 15, 15, 15, 50, 15, 15, 15, 80, 10, 50, 10, 50, 10, 10, 10},
  {90, 15, 15, 15, 50, 15, 15, 15, 80, 10, 50, 10, 50, 10, 10, 10},
  {90, 15, 15, 15, 50, 15, 15, 15, 80, 10, 50, 10, 50, 10, 10, 10},
  {90, 15, 15, 15, 50, 15, 15, 15, 80, 10, 50, 10, 50, 10, 10, 10},
};

void setup()
{
  Serial.begin(115200);
  Serial.println();

  // connect to wifi
  // Initialise wifi connection
  wifiConnected = connectWifi();

  // only proceed if wifi connection successful
  if (wifiConnected) {
    Serial.println();

    Serial.print("Hooray! Connected to ");
    Serial.println(ssid);
    Serial.print("I'm using IP address: ");
    Serial.println(WiFi.localIP());

    udpConnected = connectUDP();
    if (udpConnected) {
      // initialise pins

      // initialize output pins to control the motors
      for (int i = 0; i < nNotes; i++) {
        pinMode(pins[i], OUTPUT);
        noteHigh[i] = false;
        lastHit[i] = 0;
      }
    }
  }
}
void loop() {
  // for all motor driver pins
  // check if onTime since lastHit has elapsed
  for (int i = 0; i < nNotes; i++) {
    // if so, turn the motor off
    if (noteHigh[i] && (millis() - lastHit[i] > onTime)) {
      noteHigh[i] = false;
      analogWrite(pins[i], 0);
    }
  }

  // check if the WiFi and UDP connections were successful
  if (wifiConnected) {
    if (udpConnected) {

      // if there’s data available, read a packet
      int packetSize = UDP.parsePacket();
      if (packetSize)
      {
        Serial.println("");
        Serial.print("Received packet of size ");
        Serial.println(packetSize);
        Serial.print("From ");
        IPAddress remote = UDP.remoteIP();
        for (int i = 0; i < 4; i++)
        {
          Serial.print(remote[i], DEC);
          if (i < 3)
          {
            Serial.print(".");
          }
        }
        Serial.print(", port ");
        Serial.println(UDP.remotePort());

        // read the packet into packetBufffer
        UDP.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
        int beatStep = packetBuffer[0] * 256 + packetBuffer[1];
        Serial.print("got message: ");
        Serial.println(beatStep);

        // check if in range
        if (beatStep >= 0 && beatStep < nBeats) {

          checkSensorAndPlay(beatStep);

        }
      }
    }
  }
}

void checkSensorAndPlay(int beatStep) {

  // read the sensor
  int sensorValue = analogRead(A0);
  Serial.print(" sensor: ");
  Serial.println(sensorValue);

  // closest value -
  if (sensorValue > 700) {
    for (int i = 0; i < nNotes; i++) {
      // play pattern 2
      Serial.print("pattern 2, setting analog pin: ");
      Serial.print(pins[i]);
      Serial.print(" to value: ");
      Serial.println(pattern[0][i][beatStep]);
      analogWrite(pins[i], pattern[2][i][beatStep]);
      // keep track of which note is high and reset the time since the lastHit to now
      noteHigh[i] = true;
      lastHit[i] = millis();
    }
  }
  else if (sensorValue > 500) {
    for (int i = 0; i < nNotes; i++) {
      // play pattern 2
      Serial.print("pattern 1, setting analog pin: ");
      Serial.print(pins[i]);
      Serial.print(" to value: ");
      Serial.println(pattern[0][i][beatStep]);
      analogWrite(pins[i], pattern[1][i][beatStep]);
      // keep track of which note is high and reset the time since the lastHit to now
      noteHigh[i] = true;
      lastHit[i] = millis();
    }
  }
  else if (sensorValue > 350) {
    for (int i = 0; i < nNotes; i++) {
      // play pattern 0
      Serial.print("pattern 0, setting analog pin: ");
      Serial.print(pins[i]);
      Serial.print(" to value: ");
      Serial.println(pattern[0][i][beatStep]);
      analogWrite(pins[i], pattern[0][i][beatStep]);
      // keep track of which note is high and reset the time since the lastHit to now
      noteHigh[i] = true;
      lastHit[i] = millis();
    }
  }
  else {
    // play nothing
  }

}

// connect to UDP – returns true if successful or false if not
boolean connectUDP() {
  boolean state = false;

  Serial.println("");
  Serial.println("Connecting to UDP");

  if (UDP.begin(localPort) == 1) {
    Serial.println("Connection successful");
    state = true;
  }
  else {
    Serial.println("Connection failed");
  }
  return state;
}

// connect to wifi – returns true if successful or false if not
boolean connectWifi() {
  boolean state = true;
  int i = 0;
  WiFi.begin(ssid, password);
  WiFi.config(ip, gateway, subnet);
  Serial.println("");
  Serial.println("Connecting to WiFi");

  // Wait for connection
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (i > 10) {
      state = false;
      break;
    }
    i++;
  }
  if (state) {
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  else {
    Serial.println("");
    Serial.println("Connection failed.");
  }
  return state;
}
