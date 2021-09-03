//--- WEB Server --------------------------------------------------------------------------
//#include <ESP8266WebServer.h>
//ESP8266WebServer server(80);
// переменная хранит в себе ответ сервера в виду XML документа
String XML;
String connectResponse = ""
                         "<!DOCTYPE html><html lang='en'><head>"
                         //"<meta http-equiv=\"refresh\" content =\"1;URL=http://172.217.28.1/ \"/>"
                         "<meta name='viewport' content='width=device-width'>"
                         "<title>Upload File</title></head><body>"
                         "<BR><a href='/'>Main page. </a><BR>\n"
                         "<BR><a href='/device.htm'>Settings page</a><BR>\n"
                         "<h1>Please input WiFi data</h1>"
                         "<form action=\"/setNetwork\">"
                         "<label for=\"SSID\">WiFi SSID:</label><br>"
                         "<input type=\"text\" id=\"SSID\" name=\"SSID\"><br>"
                         "<label for=\"passwd\">Password:</label><br>"
                         "<input type=\"text\" id=\"passwd\" name=\"passwd\"><br><br>"
                         "<input type=\"submit\" value=\"Submit\">"
                         "</form>"
                         "</body></html>";
const char* serverIndex = "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";
int i; //counter for sensors
String buildWebsite(bool flag) {
  String webSite = "";
  webSite = "<!DOCTYPE HTML>\n";
  webSite += "<title >Update</title>";
  webSite += "<meta http-equiv='Content-type' content='text/html; charset=utf-8'>";
  webSite += "<BODY>\n";
  if (flag) {
    webSite += "<h3>UPDATE ERROR</h3>";
  }
  else {
    webSite += "<h3>Update OK.</h3>";
  }
  webSite += "<BR><a href='/'>Main page. </a><BR>\n";
  webSite += "<BR><a href='/device.htm'>Settings page. </a><BR>\n";
  webSite += "</BODY>";
  webSite += "</HTML>";
  return webSite;
}
void handleConnect () {
  Serial.println("Switch MODE TO WIFI_MODE_APSTA");
  server.send(200, "text / plain", connectResponse); // Oтправляем ответ
}

// web сервер с авторизацией
void webServer_init()
{
  server.on("/generate_204", handleWebsite);  //Android captive portal. Maybe not needed. Might be handled by notFound handler.
  server.on("/", handleWebsite);
  server.on("/reboot", handleReboot );
  server.on("/xml", handleXML);
  server.on("/win", handleWin);
  server.on("/restart", handlerestart);
  server.on("/hotspot-detect.html", handleWebsite);
  server.on("/library/test/success.html", handleWebsite);
  server.on("/setNetwork", handleSetNetwork);
  server.on("/connect", handleConnect);
  server.on("/redirect", handleWebsite);
  //server.on("/set", handleSet);
  server.on("/uploadfile", HTTP_POST, []() {
    server.send(200, "text/plain", "");
  }, handleFileUpload);

  server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });
  server.on("/update", HTTP_POST, []() {
    //client.disconnect();
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", buildWebsite(Update.hasError()));

    delay (200);
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.setDebugOutput(true);
      //WiFiUDP::stopAll();
      Serial.printf("Update: %s\n", upload.filename.c_str());

      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      if (!Update.begin(maxSketchSpace)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
      Serial.print ("# ");
      digitalWrite (DEBUG_LED_PIN, LOW);
      delay(2);
      digitalWrite (DEBUG_LED_PIN, HIGH);
    } else if (upload.status == UPLOAD_FILE_END) {
      Serial.println("");
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
      Serial.setDebugOutput(false);
    }
    yield();
  });


  server.begin();
}

//
void handleWebsite() {

  if (!handleFileRead("/")) server.send(200, "text/html", notFoundResponse);
}

void handleWriteCombo() {
  //writeCombo();
  server.send(200, "text / plain", "WRITE_OK"); // Oтправляем ответ Reset OK
}

// метод возвращает время вида чч:мм:сс получая на вход секунды
String millis2WorkTime(unsigned long sec, byte GMT)
{
  if ( sec == 0 )
    return "--:--:--";
  byte tmp = ( sec  % 86400L) / 3600 + GMT; // GMT +3
  String tmpTime = "";
  if (tmp > 23)
    tmp = tmp - 24;
  if ( tmp < 10)
    tmpTime = "0" + String(tmp);
  else
    tmpTime = String(tmp);
  //local_hour = tmpTime.toInt();
  //tcpLogString = String(local_hour);
  tmpTime += ":";
  tmp = (sec  % 3600) / 60 ;
  if ( tmp < 10)
    tmpTime += "0" + String(tmp);
  else
    tmpTime += String(tmp);
  tmpTime += ":";
  tmp = (sec  % 60);
  if ( tmp < 10)
    tmpTime += "0" + String(tmp);
  else
    tmpTime += String(tmp);
  return tmpTime;
}

void buildXML() {
  XML = "<?xml version='1.0'?>";
  XML += "<Main>";
  XML += "<r>";
  if (WiFi.SSID().length() == 0)
    XML += ("hot spot mode");
  else
    XML += (WiFi.SSID());
  XML += "</r>";
  XML += "<r1>";
  XML += WiFi.RSSI();
  XML += "dB";
  XML += "</r1>";
  XML += "<fH>";
  XML += ESP.getFreeHeap();
  XML += "</fH>";
  XML += "<wT>";
  XML += (millis2WorkTime(millis() / 1000, 0 ));
  XML += "</wT>";
  XML += "<pjN>";
  XML += PROJECT_NAME;
  XML += "</pjN>";
  XML += "<prN>";
  XML += PROP_NAME;
  XML += "</prN>";
  XML += "<date>";
  XML += _DATE_;
  XML += "</date>";
  XML += "<file>";
  XML += _FILE_;
  XML += "</file>";
  XML += "<time>";
  XML += _TIME_;
  XML += "</time>";
  XML += "<cntWiFiErr>";
  XML += String(cntWiFiErr);
  XML += "</cntWiFiErr>";
  XML += "<cntMQTTErr>";
  XML += String(cntMQTTErr);
  XML += "</cntMQTTErr>";
  XML += "<board>";
  XML += _BOARD_;
  XML += "</board>";
  for (int i = 0; i < COMBOCOUNT; i++) {
    XML += "<combo";
    XML += i+1;
    XML += ">";
    XML += isComboWin[i]? "WIN" : "WAIT";
    XML += "</combo";
    XML += i+1;
    XML += ">";
  }
  for (int i = 0; i < COMBOCOUNT; i++) {
    XML += "<comboV";
    XML += i+1;
    XML += ">";
    for(int x = 0; x < DIGITSINONECOMBO; x++) {
      XML += combo[i][x];
      if(x < DIGITSINONECOMBO - 1)
        XML += ",";
    }
    XML += "</comboV";
    XML += i+1;
    XML += ">";
    
  }
  for(int i = 0; i < SWITCHERSCOUNT; i++) {
    XML += "<switch";
    XML += i;
    XML += ">";
    XML += isSwitchOn[i] ? "1" : "0";
    XML += "</switch";
    XML += i;
    XML += ">";
  }
    
  XML += "<switchState>";
  XML += switchState;
  XML += "</switchState>";
  XML += "<game_state>";
  XML += isWin ? "WIN" : "PLAY";
  XML += "</game_state>";
  XML += "</Main>";

}


String millis2time1() {
  String Time = "";
  Time += "Сеть: \"";
  Time += WiFi.SSID();
  Time += "\"   Уровень сигнала: ";
  Time += WiFi.RSSI();
  Time += "dB";
  return Time;
}

void handleXML() {
  buildXML();
  server.send(200, "text/xml", XML);
}

// Перезагрузка модуля по запросу вида http://192.168.0.101/restart?device=ok
void handleReboot() {
  String restart = server.arg("device");          // Получаем значение device из запроса
  if (restart == "ok") {                         // Если значение равно Ок
    server.send(200, "text / plain", "Reset OK"); // Oтправляем ответ Reset OK
    delay (500);
    ESP.restart();                                // перезагружаем модуль
    delay (1000);
  }
  else {                                        // иначе
    server.send(200, "text / plain", "No Reset"); // Oтправляем ответ No Reset
  }
}

void handleSetNetwork () {
  if (server.hasArg("SSID")) {
    ssid = server.arg("SSID");
  }
  else {
    server.send(403, "text / plain", "SSID DATA ERR"); // Oтправляем ответ
    return;
  }
  if (server.hasArg("passwd")) {
    password = server.arg("passwd");
  }
  else {
    server.send(403, "text / plain", "PASSWORD DATA ERR"); // Oтправляем ответ
    return;
  }

  WiFi.mode(WIFI_MODE_APSTA);
  delay(100);
  WiFi.begin(ssid.c_str(), password.c_str());
  byte cnt = 0;
  while (WiFi.status() != WL_CONNECTED && cnt < 30)   {
    delay(500);
    cnt++;
    Serial.println("Connecting to WiFi..");
  }
  if (WiFi.status() != WL_CONNECTED) {
    server.send(200, "text / plain", connectResponse); // Oтправляем ответ
    return;
  }

  Serial.print("ESP32 IP as soft AP: ");
  Serial.println(WiFi.softAPIP());

  Serial.print("ESP32 IP on the WiFi network: ");
  Serial.println(WiFi.localIP());
  String answer = "<a target='_blank' href='http://" + WiFi.localIP().toString() + "'>" + WiFi.localIP().toString() + "</a>\n";
  server.send(200, "text / plain", answer); // Oтправляем ответ
}

void handleWin() {
  Win();
  server.send(200, "text / plain", "WIN_OK"); // Oтправляем ответ  OK
}


void handlerestart() {
  restart();
  server.send(200, "text / plain", "Restart_OK"); // Oтправляем ответ Restart OK
}

void handleSet() {
  //setNewCombo();
  server.send(200, "text / plain", "Set_OK"); // Oтправляем ответ  OK
}
