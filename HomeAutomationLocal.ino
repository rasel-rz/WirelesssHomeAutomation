#include <SPI.h>
#include <Ethernet.h>
#include <EthernetClient.h>

const int numOfDevices = 4;
int devices[numOfDevices] = {22, 24, 26, 28};
int deviceOffline[numOfDevices] = {23, 25, 27, 29};
bool stateOffline[numOfDevices] = {false, false, false, false};

#define MAX_STRING_LEN  20
char tagStr[MAX_STRING_LEN] = "";
char dataStr[MAX_STRING_LEN] = "";
char tmpStr[MAX_STRING_LEN] = "";
char endTag[3] = {'<', '/', '\0'};
int len;
int count = 0;
boolean tagFlag = false;
boolean dataFlag = false;

byte mac[] = { 0x30, 0x10, 0xB3, 0x48, 0x9A, 0x96 };
IPAddress ip(192, 168, 0 , 200);
byte gateway[] = { 192, 168, 0, 1 };
byte subnet[] = {255, 255, 255, 0};

byte server[] = { 192, 168, 0, 100};
//char server[] = "";

EthernetClient client;

void setup() {
  Serial.begin(9600);
  Serial.println("Connecting...");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip);
  }
  for (int i = 0; i < numOfDevices; i++) {
    pinMode(devices[i], OUTPUT);
    pinMode(deviceOffline[i], INPUT);
  }
  delay(1000);
}

int x = 0;
void loop() {
  if (client.connect(server, 80)) { //starts client connection, checks for connection
    Serial.println("connected");
    client.println("GET /test_IOT/test.xml HTTP/1.1");
    client.println( "Host:  192.168.0.100");
    client.println();
    client.println("Connection: close");
  }
  else {
    Serial.println("connection failed"); //error message if no client connect
    Serial.println();
  }
  while (client.connected() && !client.available()) delay(1); //waits for data
  while (client.connected() || client.available()) {
    xmlread();
  }
  client.stop();
  client.flush();
  Serial.println("Disconnecting");
  //delay(250);
  physicalControl();
  x++;
  Serial.println(x);
}

String tempStr;

void physicalControl() {
  Serial.println("......................................................It goes well.");
  for (int n = 0; n < numOfDevices; n++) {
    if (digitalRead(deviceOffline[n]) == HIGH) {
      delay(300);
      Serial.println("Changed");
      stateOffline[n] = !stateOffline[n];
      if (stateOffline[n]) {
        Serial.println("Make HIGH");
        switch (n) {
          case 0:
            tempStr = "state=ON&number=0";
            break;
          case 1:
            tempStr = "state=ON&number=1";
            break;
          case 2:
            tempStr = "state=ON&number=2";
            break;
          case 3:
            tempStr = "state=ON&number=3";
        }
      } else {
        Serial.println("Make LOW");
        switch (n) {
          case 0:
            tempStr = "state=OFF&number=0";
            break;
          case 1:
            tempStr = "state=OFF&number=1";
            break;
          case 2:
            tempStr = "state=OFF&number=2";
            break;
          case 3:
            tempStr = "state=OFF&number=3";
        }
      }
      if (client.connect(server, 80)) { //starts client connection, checks for connection
        Serial.println("connected...............2nd time");
      }
      else {
        Serial.println("connection failed.................2nd Time"); //error message if no client connect
        Serial.println();
      }
      Serial.println(".........................Reading data from offline...........");
      if (client.connected() || client.available()) {
        Serial.println("Really");
        client.print("GET /test_IOT/xmlupdate.php?");
        client.print(tempStr);
        client.println("HTTP/1.1");
        client.println("Content-Type: application/x-www-form-urlencoded");
        client.println( "Host:  192.168.0.100");
        //client.println("Cache-Control: no-cache");
        //client.println("User-Agent: Arduino/1.0");
        //client.println("Accept: */*");
        client.println("Connection: close");
        //client.print("Content-Length: ");
        //client.println(tempStr.length());
        //Serial.println(tempStr);
        //client.println();
        
        client.println();
        //client.println("Connection: close");
        client.stop();
        Serial.println("Disconnecting................................2nd Time");
        client.flush();
      }
    }
  }
}
void xmlread() {

  char inChar = client.read();

  if (inChar == '<') {
    addChar(inChar, tmpStr);
    tagFlag = true;
    dataFlag = false;
  } else if (inChar == '>') {
    addChar(inChar, tmpStr);
    if (tagFlag) {
      strncpy(tagStr, tmpStr, strlen(tmpStr) + 1);
    }
    clearStr(tmpStr);
    tagFlag = false;
    dataFlag = true;
  } else if (inChar != 10) {
    if (tagFlag) {
      addChar(inChar, tmpStr);
      if ( tagFlag && strcmp(tmpStr, endTag) == 0 ) {
        clearStr(tmpStr);
        tagFlag = false;
        dataFlag = false;
      }
    }
    if (dataFlag) {
      addChar(inChar, dataStr);
    }
  }


  if (inChar == 10 ) {
    if (matchTag("<state>")) {

      //Serial.print("state: ");
      //Serial.println(dataStr);
      //Serial.print("length--");
      //Serial.println(strlen(dataStr));
      count = count + 1;
      //Serial.println(count);
      devicescontrol(count, dataStr);
      if (count == 4) {
        count = 0;

        clearStr(tmpStr);
        clearStr(tagStr);
        clearStr(dataStr);
        //client.stop();
        //client.flush();
        //Serial.println("disconnecting.");
      }
    }
    clearStr(tmpStr);
    clearStr(tagStr);
    clearStr(dataStr);
    tagFlag = false;
    dataFlag = false;
  }
}


void clearStr (char* str) {
  int len = strlen(str);
  for (int c = 0; c < len; c++) {
    str[c] = 0;
  }
}


void addChar (char ch, char* str) {
  char *tagMsg  = "<ERROR>";
  char *dataMsg = "-ERROR";


  if (strlen(str) > MAX_STRING_LEN - 2) {
    if (tagFlag) {
      clearStr(tagStr);
      strcpy(tagStr, tagMsg);
    }
    if (dataFlag) {
      clearStr(dataStr);
      strcpy(dataStr, dataMsg);
    }
    clearStr(tmpStr);
    tagFlag = false;
    dataFlag = false;

  }
  else {
    str[strlen(str)] = ch;
  }
}


boolean matchTag (char* searchTag) {
  if ( strcmp(tagStr, searchTag) == 0 ) {
    return true;
  } else {
    return false;
  }
}


void devicescontrol(int devicescount, char *devicestate) {
  if (!strncmp("ON", devicestate, 2)) {
    //Serial.print("on..........on......");
    //Serial.println(devicescount);
    digitalWrite(devices[devicescount - 1], HIGH);
    stateOffline[devicescount-1] = true;
    updateSerialMonitor(devicescount, true);
  }
  else if (!strncmp("OFF", devicestate, 3)) {
    //Serial.print("off......off.......");
    //Serial.println(devicescount);
    digitalWrite(devices[devicescount - 1], LOW);
    stateOffline[devicescount-1] = false;
    updateSerialMonitor(devicescount, false);
  }
}


void updateSerialMonitor(int deviceNo, bool state) {
  switch (deviceNo) {
    case 1:
      Serial.print(">Switch-1 \t");
      dispState(state);
      break;
    case 2:
      Serial.print(">Switch-2 \t");
      dispState(state);
      break;
    case 3:
      Serial.print(">Switch-3 \t");
      dispState(state);
      break;
    case 4:
      Serial.print(">Switch-4 \t");
      dispState(state);
  }
}

void dispState(bool state) {
  if (state) Serial.println("ON");
  else Serial.println("OFF");
}