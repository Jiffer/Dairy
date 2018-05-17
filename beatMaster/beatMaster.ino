#include <ESP8266WiFi.h>
#include <WiFiUDP.h>

// WiFi variables
const char* ssid = "tick"; // ssid
const char* password = "boomboom";// password
boolean wifiConnected = false;
IPAddress ip(10, 0, 0, 100);
IPAddress gateway(10, 0, 0, 1);
IPAddress subnet(255, 255, 255, 0);

// send to:
IPAddress remoteList[2]=
{
  IPAddress(10,0,0,101),
  IPAddress(10,0,0,102)
};

//remoteList = new IPAdress(10,0,0,101);
IPAddress remote(10, 0, 0, 101);
IPAddress remote2(10, 0, 0, 102);

// Time variables
int tempo = 300;
int beat = 0;
int nBeats = 16;
float swing = 0.3;
unsigned long lastBeat = 0;
boolean delayBeat = false;

// UDP variables
unsigned int localPort = 5280;
WiFiUDP UDP;
boolean udpConnected = false;
char packetBuffer[UDP_TX_PACKET_MAX_SIZE]; //buffer to hold incoming packet,
char ReplyBuffer[] = "acknowledged"; // a string to send back

void setup() {
  // Initialise Serial connection
  Serial.begin(115200);

  // Initialise wifi connection
  wifiConnected = connectWifi();

  // only proceed if wifi connection successful
  if (wifiConnected) {
    udpConnected = connectUDP();
    if (udpConnected) {
      // initialise pins
      pinMode(LED_BUILTIN, OUTPUT);
    }
  }
}

void loop() {
  // check if the WiFi and UDP connections were successful
  if (wifiConnected) {
    if (udpConnected) {

      // TODO: make it use millis() instead of delay()
      if ((millis() - lastBeat) > tempo) {

        lastBeat = millis();
        tempo = 300;
        if (delayBeat) {
          tempo = tempo + swing * tempo;
        }
        else {
          tempo = tempo - swing * tempo;
        }

        delayBeat = !delayBeat;

        Serial.print("send beat number:");
        Serial.println(beat);
        byte message[2];
        message[0] = 0;
        message[1] = beat++;
        beat %= nBeats;

        // blink the beat
        digitalWrite(LED_BUILTIN, (beat % 2));

        UDP.beginPacket(remoteList[0], localPort);
        UDP.write(message, sizeof(message));
        int success = UDP.endPacket();

        if (success != 1) {
          Serial.print("uh oh, got: ");
          Serial.println(success);
          Serial.print("resetarting udp");
          udpReset();
        }
      }
    }
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
    Serial.print("Try ");
    Serial.println(i);
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

void udpReset() {
  // asm volatile ("  jmp 0");
  if (wifiConnected) {
    udpConnected = connectUDP();
    if (udpConnected) {
      Serial.print(" ...success");
    }
    else {
      Serial.print(" on nose!");

    }
  }
}

