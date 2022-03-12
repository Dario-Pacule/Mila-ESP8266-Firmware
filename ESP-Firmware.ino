#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>

//============== RTC Config ====================//
#define countof(a) (sizeof(a) / sizeof(a[0]))

//Pins declareit
ThreeWire myWire(4, 5, 2); // IO, SCLK, CE
RtcDS1302<ThreeWire> Rtc(myWire);


//Extern parameters initialize
extern RtcDateTime holdTime; //RTC

extern void rtcSetup(); //RTC
extern void rtcOperation(); // RTC
extern void printDateTime(const RtcDateTime& dt); //RTC
extern void serialPrint(const char printTyp, const String prop);
extern int durationCalculator (const RtcDateTime& dt);

//=============== Server Config ================//

// Configure Wifi access End-Point credentials
const char* ssid = "iPhone de Dario";
const char* password = "20192023";

// Variables declarations
byte configHour[3], configMin[3];
int sensorLevel, counterID = 0, duration[3];
String operationId[3], operationType[3];
String operatioLog = "";
boolean testStatus = true, sensorIrrigStatus = false, timeIrrigStatus[3] = {false, false, false};
boolean operationStatus[3] = {true, true, true};
boolean auxBoolean = true, auxBoolean2 = true, auxBoolean3 = true, auxBoolean4 = true, auxBoolean5 = true; //Auxiliar boolean variable

RtcDateTime holdTime2, holdTime3, holdTime4;

// Pins COnfig
const byte testPinOUTPUT = 16;

const byte relay1 = 10;
const byte sensor1 = A0;

#define ACTIVE LOW
#define DISABLE HIGH
#define ACTIVE_RELAY HIGH
#define DISABLE_RELAY LOW

// Server object instance
ESP8266WebServer server(80);


//======================================================//
//================== Function prototype ================//
//======================================================//

void setConfig();
void getConfigs();
void printOperatioLog();
void restServerRouting();
void sendCrossOriginHeader();
void setCrossOrigin();
void handleNotFound();
void regOperatioLog(const char type, const RtcDateTime& dt, byte confHour, byte confMin, byte logDuration);
void testModeStatus();
void executeIrrigation(boolean test);
void testIrrigation();

//======================================================//
//======================== Setup =======================//
//======================================================//
void setup(void) {
  //Pins Definition
  pinMode(relay1, OUTPUT);
  pinMode(testPinOUTPUT, OUTPUT);

  digitalWrite(relay1, DISABLE_RELAY);
  digitalWrite(testPinOUTPUT, ACTIVE);

  // WiFi inicialisation
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
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

  // Activate mDNS this is used to be able to connect to the server
  // with local DNS hostmane esp8266.local
  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  // Set server routes
  restServerRouting();
  server.onNotFound(handleNotFound);

  // Start server
  server.begin();
  Serial.println("HTTP server started");

  // RTC Module Initialise
  rtcSetup();
}


//======================================================//
//======================= LOOP =========================//
//======================================================//
void loop(void) {
  server.handleClient(); // Server operations
  rtcOperation(); //Send time to Client and print on Serial monitor

  if (configHour[0] != NULL || configHour[1] != NULL || configHour[2] != NULL) {
    executeIrrigation(testStatus); //Irrigate operation
  } else {
    if (auxBoolean) {
      Serial.println("Irrigation not started!");
      Serial.println("Missing time setting!");

      auxBoolean = false;
    } else {
      delay(500);
      Serial.print(".");
    }
  }
}



//======================================================//
//============== Configure irrigation operations ========//
//======================================================//
void setConfig() {
  setCrossOrigin();// CORS

  // Reception of mobile system configuration
  String irrigTime = server.arg("time");                   // Example "12:32"
  String opType = server.arg("operation");                 // Example "Irrigacao"
  String opId = server.arg("id");                          // Example "1"
  int opDuration = server.arg("duration").toInt();         // Example "60" minutes


  if (opId.equals("1")) {           // Add config if ID is 1
    operationId[0] = opId;
    operationType[0] = opType;
    duration[0] = opDuration;
  } else if (opId.equals("2")) {    // Add config if ID is 2
    operationId[1] = opId;
    operationType[1] = opType;
    duration[1] = opDuration;
  } else if (opId.equals("3")) {    // Add config if ID is 3
    operationId[2] = opId;
    operationType[2] = opType;
    duration[2] = opDuration;
  }

  server.send(200, "text/json", "[{\"id\": \"" + opId + "\",\"time\": \"" + irrigTime + "\",\"operation\": \"" + opType + "\",\"sensorLevel\": \"" + sensorLevel + "\"}]");


  //Time formating
  boolean stat = true;
  boolean stat2 = true;
  byte cont = 0;
  byte confH = 0, confM = 0;

  while (1) {
    if (cont < 2) {
      if (stat) {
        confH = 10 * (irrigTime[cont] - '0');
        stat = false;
      } else {
        confH += (irrigTime[cont] - '0');
      }
    } else {
      if (cont != 2) {
        if (stat2) {
          confM = 10 * (irrigTime[cont] - '0');
          stat2 = false;
        } else {
          confM += (irrigTime[cont] - '0');
          break;
        }
      }
    }
    cont++;
  }


  if (opId.equals("1")) {           // Add config if ID is 1
    configHour[0] = confH;
    configMin[0] = confM;
  } else if (opId.equals("2")) {    // Add config if ID is 2
    configHour[1] = confH;
    configMin[1] = confM;
  } else if (opId.equals("3")) {    // Add config if ID is 3
    configHour[2] = confH;
    configMin[2] = confM;
  }

  Serial.println("============ Body Start =========");
  Serial.print("id: ");
  Serial.println(opId);
  Serial.print("Hora: ");
  Serial.println(confH);
  Serial.print("Minuto: ");
  Serial.println(confM);
  Serial.print("Sensor Level: ");
  Serial.println(sensorLevel);
  Serial.print("Duration: ");
  Serial.println(opDuration);
  Serial.println("============ Body END =========");
  Serial.println();

}//End setConfig

//======================================================//
//================== Set sensor level ==================//
//======================================================//
void setSensorLevel() {
  setCrossOrigin();
  String buff = "Sensor level configured: ";

  sensorLevel = server.arg("level").toInt();         // Example "65" %

  buff += sensorLevel;
  buff += "!";

  server.send(200, "txt/plain", buff);
}


//======================================================//
//================= Load Irrigation Config ==============//
//======================================================//
void getConfigs() {
  setCrossOrigin();

  String response = "[";
  response += "{";
  response += "\"id\": \"";
  response += operationId[0];
  response += "\"";
  response += ",\"operation\": \"";
  response += operationType[0];
  response += "\"";
  response += ",\"time\": \"";
  response += configHour[0];
  response += ":";
  response += configMin[0];
  response += "\"";

  response += "}";

  response += ",{";
  response += "\"id\": \"";
  response += operationId[1];
  response += "\"";
  response += ",\"operation\": \"";
  response += operationType[1];
  response += "\"";
  response += ",\"time\": \"";
  response += configHour[1];
  response += ":";
  response += configMin[1];
  response += "\"";

  response += "}";

  response += ",{";
  response += "\"id\": \"";
  response += operationId[2];
  response += "\"";
  response += ",\"operation\": \"";
  response += operationType[2];
  response += "\"";
  response += ",\"time\": \"";
  response += configHour[2];
  response += ":";
  response += configMin[2];
  response += "\"";

  response += "}";

  response += "]";

  server.send(200, "text/json", response);

  Serial.println();
  Serial.println("========================");
  Serial.println("Current configuration sent to Client!");
  Serial.println("========================");
  Serial.println();

  delay(500);

}

//======================================================//
//================ Send operatioLog to client ==========//
//======================================================//
void printOperatioLog() {
  setCrossOrigin();

  String rel = operatioLog;
  rel += "]";
  server.send(200, "text/json", rel);


  Serial.println();
  Serial.println("========================");
  Serial.println("Operation Log sent to the Client!");
  Serial.println("========================");
  Serial.println();

  delay(1000);
}

//======================================================//
//=================== Get Sensor level =================//
//======================================================//
void getSensorValue() {
  setCrossOrigin();// CORS

  float sensorRead = 0;
  for (int i = 0; i < 50; i++) {
    sensorRead += analogRead(sensor1);
  }

  sensorRead = sensorRead / 50;
  sensorRead = map(sensorRead, 0, 1023, 0, 100);


  String buff;
  StaticJsonDocument<20> doc;

  doc["level"] = sensorRead;
  serializeJson(doc, buff);

  server.send(200, "text/json", buff);

  Serial.println();
  Serial.println("========================");
  Serial.print("Sensor level: ");
  Serial.print(sensorRead);
  Serial.println(" sent to the Client!");
  Serial.println("========================");
  Serial.println();
}



//======================================================//
//===================== Get RTC time ===================//
//======================================================//
void rtctime() {
  setCrossOrigin();// CORS

  char datestring[6];
  String buff;
  StaticJsonDocument<200> doc;
  RtcDateTime now = Rtc.GetDateTime();

  snprintf_P(datestring,
             countof(datestring),
             PSTR("%02u:%02u"),
             now.Hour(),
             now.Minute()
            );

  doc["rtctime"] = datestring;
  serializeJson(doc, buff);

  server.send(200, "text/json", buff);

  Serial.println();
  Serial.println("========================");
  Serial.println("RTC time sent to the Client!");
  Serial.println("========================");
  Serial.println();
}

//======================================================//
//============== Systema operation mode ================//
//======================================================//
void testModeStatus() {
  setCrossOrigin();
  String buff = "Success test mode is ";

  String styt = server.arg("teststatus");

  if (styt.equals("true")) {
    testStatus = true;
  } else {
    testStatus = false;
  }

  buff += testStatus;
  buff += " now!";

  server.send(200, "txt/plain", buff);
}

//======================================================//
//=============== Define Server routes =================//
//======================================================//
void restServerRouting() {
  server.on("/", HTTP_OPTIONS, sendCrossOriginHeader);
  server.on("/", HTTP_GET, []() {
    setCrossOrigin();
    server.send(200, F("text/html"),
                F("Voce se conectou ao Sistema de Irrigacao MILA"));
  });

  server.on(F("/setconfig"), HTTP_OPTIONS, sendCrossOriginHeader);
  server.on(F("/setconfig"), HTTP_POST, setConfig);

  server.on(F("/getsensorlevel"), HTTP_OPTIONS, sendCrossOriginHeader);
  server.on(F("/getsensorlevel"), HTTP_GET, getSensorValue);

  server.on(F("/getconfigs"), HTTP_OPTIONS, sendCrossOriginHeader);
  server.on(F("/getconfigs"), HTTP_GET, getConfigs);

  server.on(F("/sensorlevel"), HTTP_OPTIONS, sendCrossOriginHeader);
  server.on(F("/sensorlevel"), HTTP_POST, setSensorLevel);

  server.on(F("/testmode"), HTTP_OPTIONS, sendCrossOriginHeader);
  server.on(F("/testmode"), HTTP_POST, testModeStatus);

  server.on(F("/log"), HTTP_OPTIONS, sendCrossOriginHeader);
  server.on(F("/log"), HTTP_GET, printOperatioLog);

  server.on(F("/rtctime"), HTTP_OPTIONS, sendCrossOriginHeader);
  server.on(F("/rtctime"), HTTP_GET, rtctime);

}

//======================================================//
//=============== CORS problem prevent =================//
//======================================================//
void sendCrossOriginHeader() {
  Serial.println(F("sendCORSHeader"));
  server.sendHeader(F("access-control-allow-credentials"), F("false"));
  setCrossOrigin();

  server.send(204);
}
// Set HTTP headers Cors Origin
void setCrossOrigin() {
  server.sendHeader(F("Access-Control-Allow-Origin"), F("*"));
  server.sendHeader(F("Access-Control-Max-Age"), F("600"));
  server.sendHeader(F("Access-Control-Allow-Methods"), F("PUT,POST,GET,OPTIONS"));
  server.sendHeader(F("Access-Control-Allow-Headers"), F("*"));
}

//======================================================//
//=============== Manage not found URL =================//
//======================================================//
void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

//======================================================//
//=========== Build operation Log in JSON Format ============//
//======================================================//
void regOperatioLog(const char type, const RtcDateTime& dt, byte confHour, byte confMin, byte logDuration) {
  if (operatioLog.isEmpty()) {
    operatioLog = "[";
  } else {
    operatioLog += ",";
  }

  if (type == '1') {
    counterID++;

    operatioLog += "{\"id\": ";
    operatioLog += counterID;
    operatioLog += ",";
    operatioLog += "\"Data\": \"";
    operatioLog += dt.Day();
    operatioLog += "/";
    operatioLog +=  dt.Month();
    operatioLog += "/";
    operatioLog +=  dt.Year();
    operatioLog += "\"";

    operatioLog += ",\"time\": \"";
    operatioLog +=  confHour;
    operatioLog += ":";
    operatioLog +=  confMin;
    operatioLog += "\"";

    operatioLog += ",\"duration\": \"";
    operatioLog +=  logDuration;
    operatioLog += "\"";

    operatioLog += ",\"type\" : \"TimeProgramed\"}";
  }
  if (type == '2') {
    counterID++;

    operatioLog += "{\"id\": ";
    operatioLog += counterID;
    operatioLog += ",";
    operatioLog += "\"Data\": \"";
    operatioLog += dt.Day();
    operatioLog += "/";
    operatioLog +=  dt.Month();
    operatioLog += "/";
    operatioLog +=  dt.Year();
    operatioLog += "\"";

    operatioLog += ",\"duration\": \"";
    operatioLog +=  logDuration;
    operatioLog += "\"";

    operatioLog += ",\"type\" : \"SensorLevel\"";
    operatioLog += ",\"level\": \"";
    operatioLog += sensorLevel ;
    operatioLog += "\"}";
  }
}

//======================================================//
//=========== Execute Irrigation operations ============//
//======================================================//
void executeIrrigation(boolean test) {
  if (test) {
    testIrrigation();

  } else {
    delay(500);
    irrigate();
  }
}

//======================================================//
//============== For test Irrigation values ============//
//======================================================//
void testIrrigation() {
  if (auxBoolean5) {
    Serial.println();
    Serial.print("==================");
    Serial.print("TEST MODE");
    Serial.print("==================");
    auxBoolean5 = false;
    auxBoolean3 = true;
    auxBoolean4 = true;
    digitalWrite(relay1, DISABLE_RELAY);
    operatioLog = "";
  }

  RtcDateTime now = Rtc.GetDateTime();
  float timer ;

  if (!operationId[0].equals(NULL)) {
    if (configHour[0] < 6) {
      timer = 200;
      digitalWrite(testPinOUTPUT, ACTIVE);

      if (auxBoolean2) {
        regOperatioLog('1', now, configHour[0], configMin[0], duration[0] );
        auxBoolean2 = false;
      }

      serialPrint('1', operationId[0]); // Print message on terminal
    } else if (configHour[0] < 12) {
      timer = 500;
      digitalWrite(testPinOUTPUT, ACTIVE);

      if (auxBoolean2) {
        regOperatioLog('1', now, configHour[0], configMin[0], duration[0] );
        auxBoolean2 = false;
      }

      serialPrint('1', operationId[0]); // Print message on terminal
    } else if (configHour[0] < 18) {
      timer = 800;
      digitalWrite(testPinOUTPUT, ACTIVE);

      if (auxBoolean2) {
        regOperatioLog('1', now, configHour[0], configMin[0], duration[0] );
        auxBoolean2 = false;
      }

      serialPrint('1', operationId[0]); // Print message on terminal
    } else {
      timer = 1000;
      digitalWrite(testPinOUTPUT, ACTIVE);

      if (auxBoolean2) {
        regOperatioLog('1', now, configHour[0], configMin[0], duration[0] );
        auxBoolean2 = false;
      }

      serialPrint('1', operationId[0]); // Print message on terminal
    }

  }
  if (!operationId[1].equals(NULL)) {
    if (configHour[1] < 6) {
      timer = 200;
      digitalWrite(testPinOUTPUT, ACTIVE);

      if (auxBoolean2) {
        regOperatioLog('1', now, configHour[1], configMin[1], duration[1]);
        auxBoolean2 = false;
      }

      serialPrint('1', operationId[1]); // Print message on terminal
    } else if (configHour[1] < 12) {
      timer = 500;
      digitalWrite(testPinOUTPUT, ACTIVE);

      if (auxBoolean2) {
        regOperatioLog('1', now, configHour[1], configMin[1], duration[1]);
        auxBoolean2 = false;
      }

      serialPrint('1', operationId[1]); // Print message on terminal
    } else if (configHour[1] < 18) {
      timer = 800;
      digitalWrite(testPinOUTPUT, ACTIVE);

      if (auxBoolean2) {
        regOperatioLog('1', now, configHour[1], configMin[1], duration[1]);
        auxBoolean2 = false;
      }

      serialPrint('1', operationId[1]); // Print message on terminal
    } else {
      timer = 1000;
      digitalWrite(testPinOUTPUT, ACTIVE);

      if (auxBoolean2) {
        regOperatioLog('1', now, configHour[1], configMin[1], duration[1]);
        auxBoolean2 = false;
      }

      serialPrint('1', operationId[1]); // Print message on terminal
    }
  }
  if (!operationId[2].equals(NULL)) {
    if (configHour[2] < 6) {
      timer = 200;
      digitalWrite(testPinOUTPUT, ACTIVE);

      if (auxBoolean2) {
        regOperatioLog('1', now, configHour[2], configMin[2], duration[2]);
        auxBoolean2 = false;
      }

      serialPrint('1', operationId[2]); // Print message on terminal
    } else if (configHour[2] < 12) {
      timer = 500;
      digitalWrite(testPinOUTPUT, ACTIVE);

      if (auxBoolean2) {
        regOperatioLog('1', now, configHour[2], configMin[2], duration[2]);
        auxBoolean2 = false;
      }


      serialPrint('1', operationId[2]); // Print message on terminal
    } else if (configHour[2] < 18) {
      timer = 800;
      digitalWrite(testPinOUTPUT, ACTIVE);

      if (auxBoolean2) {
        regOperatioLog('1', now, configHour[2], configMin[2], duration[2]);
        auxBoolean2 = false;
      }

      serialPrint('1', operationId[2]); // Print message on terminal
    } else {
      timer = 1000;
      digitalWrite(testPinOUTPUT, ACTIVE);

      if (auxBoolean2) {
        regOperatioLog('1', now, configHour[2], configMin[2], duration[2]);
        auxBoolean2 = false;
      }

      serialPrint('1', operationId[2]); // Print message on terminal
    }
  }

  delay(timer);
  digitalWrite(testPinOUTPUT, DISABLE);
  delay(timer);

  serialPrint('2', ""); // Print Irrigation is compomplete message on terminal
}// End testIrrigation Function


//======================================================//
//================= Irrigation function ================//
//======================================================//
void irrigate() {
  if (auxBoolean4) {
    Serial.println();
    Serial.print("==================");
    Serial.print("REAL TIME MODE");
    Serial.print("==================");
    auxBoolean5 = true;
    auxBoolean4 = false;
    operatioLog = "";
  }

  float sensorRead = 0;
  RtcDateTime now = Rtc.GetDateTime();

  //===================================
  if (!operationId[0].equals(NULL)) {
    if (now.Hour() >= configHour[0] && operationStatus[0]) {
      if (now.Minute() >= configMin[0] && auxBoolean3) {
        digitalWrite(relay1, ACTIVE_RELAY);
        timeIrrigStatus[0] = true;
        auxBoolean3 = false;

        serialPrint('1', operationId[0]); // Print message on terminal
      }

      if (durationCalculator(holdTime4) > duration[0] && timeIrrigStatus[0]) { // Control irrig duration
        operationStatus[0] = false;
      }

    } else if (sensorIrrigStatus == false && timeIrrigStatus[0] == true) {
      digitalWrite(relay1, DISABLE_RELAY);
      timeIrrigStatus[0] = false;
      auxBoolean3 = true;

      regOperatioLog('1', now, configHour[0], configMin[0], duration[0]);

      serialPrint('2', ""); // Print Irrigation is compomplete message on terminal
    }
  }

  //===================================

  if (!operationId[1].equals(NULL)) {
    if (now.Hour() >= configHour[1] && operationStatus[1]) {
      if (now.Minute() >= configMin[1] && auxBoolean3) {
        digitalWrite(relay1, ACTIVE_RELAY);
        timeIrrigStatus[1] = true;
        auxBoolean3 = false;

        holdTime4 = Rtc.GetDateTime();

        serialPrint('1', operationId[1]); // Print message on terminal
      }

      if (durationCalculator(holdTime4) > duration[1] && timeIrrigStatus[1]) { // Control irrig duration
        operationStatus[1] = false;
      }

    } else if (sensorIrrigStatus == false && timeIrrigStatus[1] == true) {
      digitalWrite(relay1, DISABLE_RELAY);
      timeIrrigStatus[1] = false;
      auxBoolean3 = true;

      regOperatioLog('1', now, configHour[1], configMin[1], duration[1]);

      serialPrint('2', ""); // Print Irrigation is compomplete message on terminal
    }
  }

  //===================================

  if (!operationId[2].equals(NULL)) {
    if (now.Hour() >= configHour[2] && operationStatus[2]) {
      if (now.Minute() >= configMin[2] && auxBoolean3) {
        digitalWrite(relay1, ACTIVE_RELAY);
        timeIrrigStatus[2] = true;
        auxBoolean3 = false;

        holdTime4 = Rtc.GetDateTime();

        serialPrint('1', operationId[2]); // Print message on terminal
      }

      if (durationCalculator(holdTime4) > duration[2] && timeIrrigStatus[2]) { // Control irrig duration
        operationStatus[2] = false;
      }

    } else if (sensorIrrigStatus == false && timeIrrigStatus[2] == true) {
      digitalWrite(relay1, DISABLE_RELAY);
      timeIrrigStatus[2] = false;
      auxBoolean3 = true;

      regOperatioLog('1', now, configHour[2], configMin[2], duration[2]);

      serialPrint('2', ""); // Print Irrigation is compomplete message on terminal
    }
  }

  // ============ Sensor based operations =========================
  // ==============================================================
  sensorRead = 0;
  for (int i = 0; i < 50; i++) {
    sensorRead += analogRead(sensor1);
  }

  sensorRead = sensorRead / 50;
  sensorRead = map(sensorRead, 0, 1023, 0, 100);
  Serial.print("Sesor value = ");
  Serial.println(sensorRead);


  if (sensorLevel > sensorRead) {
    if (auxBoolean3) {
      sensorIrrigStatus = true;
      digitalWrite(relay1, ACTIVE_RELAY);
      auxBoolean3 = false;
      holdTime2 = now;
      Serial.println();
      Serial.println("========================");
      Serial.println("Sensor level irrigation Started!");
      Serial.println();
    }
  } else if (sensorIrrigStatus == true && timeIrrigStatus[0] == false && timeIrrigStatus[1] == false && timeIrrigStatus[2] == false) {
    digitalWrite(relay1, DISABLE_RELAY);
    sensorIrrigStatus = false;
    auxBoolean3 = true;

    regOperatioLog('2', now, now.Hour(), now.Minute(), durationCalculator(holdTime2));

    serialPrint('2', ""); // Print Irrigation is compomplete message on terminal
  }

}// End Irrigation Function
