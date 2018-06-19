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
const int nBeaters = 4;
IPAddress remoteList[nBeaters] =
{
  IPAddress(10, 0, 0, 101),
  IPAddress(10, 0, 0, 102),
  IPAddress(10, 0, 0, 103),
  IPAddress(10, 0, 0, 104)
};



// Time variables
int tempo = 150;
int tempoRef = 150;
int beat = 0;

int songLength = 12;
const int nBeats = 16;
float swing = 0.0;
unsigned long lastBeat = 0;
boolean delayBeat = false;

// drum machine Variables
const int pins[] = {16, 5, 4, 0}; // labeled D0, D1, D2, D3, D4 on nodeMCU PCB
const int nNotes = 4;

// how long to pulse in ms
unsigned int onTime = 5;
// keep track of the last time a motor channel (note) was triggered
unsigned long lastHit[nNotes];
// keep track if a note is currently on (high) or if it has been released
bool noteHigh[nNotes];
int pattern[3][nNotes][nBeats] = {{
    {1000, 0, 0, 0, 1000, 0, 0, 0, 800, 0, 0, 0, 800, 0, 0, 0},
    {0, 1000, 0, 0, 0, 1000, 0, 0, 0, 800, 0, 0, 0, 800, 0, 0},
    {0, 0, 1000, 0, 0, 0, 1000, 0, 0, 0, 800, 0, 0, 0, 800, 0},
    {0, 0, 0, 1000, 0, 0, 0, 1000, 0, 0, 0, 800, 0, 0, 0, 800}
  },
  {
    {1000, 0, 0, 0, 1000, 0, 0, 0, 800, 0, 0, 0, 800, 0, 0, 0},
    {0, 1000, 0, 0, 0, 1000, 0, 0, 0, 800, 0, 0, 0, 800, 0, 0},
    {0, 0, 1000, 0, 0, 0, 1000, 0, 0, 0, 800, 0, 0, 0, 800, 0},
    {0, 0, 0, 1000, 0, 0, 0, 1000, 0, 0, 0, 800, 0, 0, 0, 800}
  },
  {
    {1000, 0, 0, 0, 1000, 0, 0, 0, 800, 0, 0, 0, 800, 0, 0, 0},
    {0, 1000, 0, 0, 0, 1000, 0, 0, 0, 800, 0, 0, 0, 800, 0, 0},
    {0, 0, 1000, 0, 0, 0, 1000, 0, 0, 0, 800, 0, 0, 0, 800, 0},
    {0, 0, 0, 1000, 0, 0, 0, 1000, 0, 0, 0, 800, 0, 0, 0, 800}
  }
};
int notesOn[nBeats] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

int liklihood[4][16] = {
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {5, 25, 25, 25, 5, 25, 25, 25, 5, 25, 25, 25, 5, 25, 25, 25},
  {50, 10, 15, 10, 15, 10, 15, 10, 40, 10, 15, 10, 20, 15, 15, 15},
  {5, 15, 10, 15, 50, 15, 15, 15, 10, 15, 10, 15, 40, 15, 15, 15},
};

int songLoopCounter = 0;
int loopCounter = 0;
int lastBeatCount = 0;
int numLoops = 10;

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
  generateSequence();
}

void loop() {

  checkOnTime();

  // check if the WiFi and UDP connections were successful
  if (wifiConnected) {
    if (udpConnected) {

      if ((millis() - lastBeat) > tempo) {
        lastBeat = millis();
        tempo = tempoRef;
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

        if (beat == 0) { // count how many times...
          songLoopCounter++;
          Serial.print(loopCounter);
          if (songLoopCounter >= songLength) {

            // load a new tempo and pause
            songLength = random(36, 60);
            tempoRef = 90 + random(100);
            swing = random(350) / 1000;
            songLoopCounter = 0;
            delay(5000);
          }
        }
        //        // blink the beat
        //        digitalWrite(16, (beat % 2));

        for (int i = 0; i < nBeaters; i++) {
          UDP.beginPacket(remoteList[i], localPort);
          UDP.write(message, sizeof(message));
          int success = UDP.endPacket();

          if (success != 1) {
            Serial.print("uh oh, got: ");
            Serial.println(success);
            Serial.print("resetarting udp");
            udpReset();
          }
        }
        // also play my motors:
        if (lastBeatCount > beat) {
          // beginning of a new loop
          // check if its time to generate new patterns
          if (loopCounter > numLoops) {
            //              Serial.println(loopCounter);
            generateSequence();
            loopCounter = 0;
          }
          loopCounter++;

        }
        lastBeatCount = beat;

        // is someone there?
        checkSensorAndPlay(beat);

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

//////////////////////////////////////////
void checkSensorAndPlay(int beatStep) {

  // read the sensor
  int sensorValue = analogRead(A0);
  //  Serial.print(" sensor: ");
  //  Serial.println(sensorValue);

  // closest value -
  if (sensorValue > 550) {
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
  else if (sensorValue > 400) {
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
  else if (sensorValue > 300) {
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

//////////////////////////////////////////
void generateSequence() {
  Serial.println("new sequence a comin");
  //    pattern[3][4][16]
  //  liklihood[4][16]

  for (int i = 0; i < nBeats; i++) {
    notesOn[i] = 0;
    for (int j = 0; j < nNotes; j++) {
      pattern[0][j][i] = 0;
      pattern[1][j][i] = 0;
      pattern[2][j][i] = 0;

      if (liklihood[j][i] > random(100) && notesOn[i] < 3) {
        notesOn[i]++;
        pattern[0][j][i] = 1023;
      }
    }
  }
  for (int i = 0; i < nBeats; i++) {
    for (int j = 0; j < nNotes; j++) {
      pattern[1][j][i] = pattern[0][j][i];
      if (pattern[1][j][i] == 0) {
        if (liklihood[j][i] > random(100) && notesOn[i] < 3) {
          notesOn[i]++;
          pattern[1][j][i] = 1023;
        }
      }
    }
  }
  for (int i = 0; i < nBeats; i++) {
    for (int j = 0; j < nNotes; j++) {
      pattern[2][j][i] = pattern[1][j][i];
      if (pattern[1][j][i] == 0) {
        if (25 > random(100) && notesOn[i] < 3) {
          notesOn[i]++;
          pattern[2][j][i] = 1023;
        }
      }
    }
  }
}

///////////////////////////////////////
void checkOnTime() {
  // for all motor driver pins
  // check if onTime since lastHit has elapsed
  for (int i = 0; i < nNotes; i++) {
    // if so, turn the motor off
    if (noteHigh[i] && (millis() - lastHit[i] > onTime)) {
      noteHigh[i] = false;
      analogWrite(pins[i], 0);
    }
  }
}
