#include <WiFi.h> // library to wifi communication/connection 
#include <WiFiServer.h> // server for tcp communicationsss
#include <WebServer.h> // server to host the webpage
#include <AsyncUDP.h> // library to send/receive udp packet
#include <EEPROM.h> // write/read flash memory
#include <heltec.h> // library to control OLED
#include "esp_wps.h" // WPS function
#include "WPS.h" // WPS function
#include "index.h" // webpage code

const int ALL_CAMS_IP_SIZE = 10;

String allCamsIP[ALL_CAMS_IP_SIZE];
const int IP_ADDRESS_SIZE = 15;
// use byte from EEPROM memory 
const int EEPROM_SIZE = (IP_ADDRESS_SIZE + 1) * ALL_CAMS_IP_SIZE;
const char* IDENTIFIER = "BA_2020/21";
const int DELAY = 5000;
WebServer serverWeb(80);
WiFiServer serverTCP(8088);

// return added index or -1
int addIPFromCam(String ip){
  IPAddress ipAddress;
  // check if ip address
  if(ipAddress.fromString(ip)){
    for (int i = 0; i < ALL_CAMS_IP_SIZE; i++){
      if(allCamsIP[i].length() == 0){
        allCamsIP[i] = ip;
        return i;
      }
    }
  }
  return -1;
}

// return removed ip value
String removeIPFromCam(int index){
  int address = 0;
  if(index < 0 || ALL_CAMS_IP_SIZE -1 < index){
    return "";
  }
  // remove ip
  String str = allCamsIP[index];
  Serial.println("Removed " + str);
  allCamsIP[index] = "";
  address = (IP_ADDRESS_SIZE + 1) * index;
  EEPROM.writeString(address, "");
  
  // shift ip to empty position
  for (int i = index; i < ALL_CAMS_IP_SIZE -1; i++){
      allCamsIP[i] = allCamsIP[i +1];
      address = (IP_ADDRESS_SIZE + 1) * i;
      EEPROM.writeString(address, allCamsIP[i +1]);
  }
  EEPROM.commit();
  
  allCamsIP[ALL_CAMS_IP_SIZE -1] = "";
  return str;
}

// join all ip and create a string in form a list - ["ip", "ip"]
String printAllCamsIP(){
  String str = "[";
  for(int i = 0; i < ALL_CAMS_IP_SIZE; i++){
    if(allCamsIP[i].length() > 0){
      str += '"' + allCamsIP[i] + '"';
      if(i < ALL_CAMS_IP_SIZE -1){
        if(allCamsIP[i +1].length() != 0){
          str += ",";
        }
      }
    }
  }
  str += "]";
  return str;
}

// return added index or -1
int getIPCam(){
  int index = -1;
  AsyncUDP udp;
  Serial.println("Start UDP");
  unsigned long startTime = millis();
  // send udp duration
  while(startTime + DELAY > millis()){
    // send broadcast on port 2032 - UDP 
    udp.broadcastTo(IDENTIFIER, 2032);
  }
  Serial.println("Start TCP");
  // listen for client ip - TCP
  startTime = millis();
  // wait for tcp connection
  while(startTime + (DELAY * 2) > millis()){
    WiFiClient client = serverTCP.available();
    uint8_t camMSG[30];
    if (client) {    
      Serial.println("Found client");     
      while (client.connected()) {   
        if (client.available()) {
          int len = client.read(camMSG, 30);
          if(len < 30){
            camMSG[len] = '\0';  
          }else {
            camMSG[30] = '\0';
          }
        } 
      }     
      if(strcmp(IDENTIFIER, (char*) camMSG) == 0){
        Serial.println(client.remoteIP().toString());
        index = addIPFromCam(client.remoteIP().toString());
        if(index > -1){
          // store ip in EEPROM
          Serial.println("Store ip in EEPROM");
          int address = (IP_ADDRESS_SIZE + 1) * index;
          EEPROM.writeString(address, client.remoteIP().toString());
          EEPROM.commit();
        }
      }
     }
  }
  return index;
}

// adjust webpage
String sendPage(int addSuccess = -2){
  // color for add button feedback
  String borderColor = "";
  switch(addSuccess){
    case -1: borderColor = "#FF3535"; break; // red
    case -2: borderColor = "#3D3F3D"; break; // black
    default: borderColor = "#04FF04"; break; // green
  }
  String index = String(INDEX);
  index.replace("BORDER_COLOR", borderColor);
  index.replace("CAM_IP_LIST", printAllCamsIP());
  return index;
}

void handleOnConnect() {
  Serial.println("Mainpage");
  serverWeb.send(200, "text/html", sendPage()); 
}

void handleNotFound(){
  String uri = serverWeb.uri();
  if(uri.equals("/add/")){
    Serial.println("Add");
    int index = getIPCam();
    serverWeb.send(200, "text/html", sendPage(index));
  }else if(uri.startsWith("/remove/")){
    Serial.println("Remove");
    int index = uri.substring(uri.lastIndexOf("/", uri.length() -1), uri.length()).toInt();
    String ip = removeIPFromCam(index);
    serverWeb.send(200, "text/html", sendPage()); 
  }else{
    Serial.println("Error");
    serverWeb.send(404, "text/plain", "Not found");
  }
}

void setup() {
  Serial.begin(115200);

  // EEPROM size to use
  if(!EEPROM.begin(EEPROM_SIZE)){
    Serial.println("failed to initialise EEPROM");
    Serial.println("Rebooting...");
    delay(10000);
    ESP.restart();
  }

  // wifi connection
  Serial.println("Starting WPS");
  WiFi.onEvent(WiFiEvent);
  WiFi.mode(WIFI_MODE_STA);
  wpsInitConfig();
  esp_wifi_wps_enable(&config);
  esp_wifi_wps_start(0);
  
  while(WiFi.status() != WL_CONNECTED){
    Serial.println("Not connected to wifi");
    delay(10000);
  }
  Serial.println("WiFi Connected");
  Serial.println(WiFi.localIP());

  // control oled and display server ip
  Heltec.begin(true, false, true);
  Heltec.display->flipScreenVertically();
  Heltec.display->clear();
  Heltec.display->setFont(ArialMT_Plain_16);
  Heltec.display->drawString(0, 16, "Server-IP:");  
  Heltec.display->drawString(0, 34, WiFi.localIP().toString());  
  Heltec.display->display();

  // get data from eeprom
  Serial.println("Get data from EEPROM");
  int address = 0;
  for(int i = 0; i < ALL_CAMS_IP_SIZE; i++){
    addIPFromCam(EEPROM.readString(address));
    address += IP_ADDRESS_SIZE + 1;
  }

  // define route and handler function for webserver
  serverWeb.on("/", handleOnConnect);
  serverWeb.onNotFound(handleNotFound);
  Serial.println("Start webserver");
  // start webserver
  serverWeb.begin();
  Serial.println("Start TCP server");
  // start TCP server
  serverTCP.begin();
}

void loop() {
  serverWeb.handleClient();
}
