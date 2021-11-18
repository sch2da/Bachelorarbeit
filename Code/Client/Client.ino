#include "esp_camera.h" // esp camera function
#include <WiFi.h> // library to wifi communication/connection 
#include <WebServer.h> // server to host the webpage
#include <AsyncUDP.h> // library to send/receive udp packet
#include "esp_wps.h" // WPS function
#include "WPS.h" // WPS function

//
// WARNING!!! Make sure that you have either selected ESP32 Wrover Module,
//            or another board which has PSRAM enabled
//

// Select camera model
//#define CAMERA_MODEL_WROVER_KIT
//#define CAMERA_MODEL_ESP_EYE
//#define CAMERA_MODEL_M5STACK_PSRAM
//#define CAMERA_MODEL_M5STACK_WIDEz
#define CAMERA_MODEL_AI_THINKER

#include "camera_pins.h" // pins for camera model

const char* IDENTIFIER = "BA_2020/21";
String serverIP = "";
bool isConnectedServerTCP = false;
int DELAY = 10000;

void startCameraServer();

void startUDP(){
  AsyncUDP udp;
  //listen UDP packet on port 2032
  if(udp.listen(2032)){
   udp.onPacket([](AsyncUDPPacket packet) {
     if(strcmp(IDENTIFIER, (char*) packet.data()) == 0){
       serverIP = packet.remoteIP().toString();
     }
   });
  }
  udp.close();
  return;
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  camera_config_t configCam;
  configCam.ledc_channel = LEDC_CHANNEL_0;
  configCam.ledc_timer = LEDC_TIMER_0;
  configCam.pin_d0 = Y2_GPIO_NUM;
  configCam.pin_d1 = Y3_GPIO_NUM;
  configCam.pin_d2 = Y4_GPIO_NUM;
  configCam.pin_d3 = Y5_GPIO_NUM;
  configCam.pin_d4 = Y6_GPIO_NUM;
  configCam.pin_d5 = Y7_GPIO_NUM;
  configCam.pin_d6 = Y8_GPIO_NUM;
  configCam.pin_d7 = Y9_GPIO_NUM;
  configCam.pin_xclk = XCLK_GPIO_NUM;
  configCam.pin_pclk = PCLK_GPIO_NUM;
  configCam.pin_vsync = VSYNC_GPIO_NUM;
  configCam.pin_href = HREF_GPIO_NUM;
  configCam.pin_sscb_sda = SIOD_GPIO_NUM;
  configCam.pin_sscb_scl = SIOC_GPIO_NUM;
  configCam.pin_pwdn = PWDN_GPIO_NUM;
  configCam.pin_reset = RESET_GPIO_NUM;
  configCam.xclk_freq_hz = 20000000;
  configCam.pixel_format = PIXFORMAT_JPEG;
  //init with high specs to pre-allocate larger buffers
  if(psramFound()){
    configCam.frame_size = FRAMESIZE_UXGA;
    configCam.jpeg_quality = 10;
    configCam.fb_count = 2;
  } else {
    configCam.frame_size = FRAMESIZE_SVGA;
    configCam.jpeg_quality = 12;
    configCam.fb_count = 1;
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&configCam);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  //initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);//flip it back
    s->set_brightness(s, 1);//up the blightness just a bit
    s->set_saturation(s, -2);//lower the saturation
  }
  //drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_QVGA);

#if defined(CAMERA_MODEL_M5STACK_WIDE)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif


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

  // LED BUILT_IN is GPIO 33 
  // pin work with inverted logic
  pinMode(33, OUTPUT); // Set the pin as output
  for(int i = 0; i < 5; i++){
    digitalWrite(33, LOW); //Turn on
    delay (250); 
    digitalWrite(33, HIGH); //Turn off
    delay (250); 
  }

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
}

void loop() {
  if(!isConnectedServerTCP){
    if(serverIP.length() == 0){
      Serial.println("Read UDP");
      startUDP();
      delay(250);
    }else{
      WiFiClient client;
      char host[serverIP.length() +1];
      serverIP.toCharArray(host, serverIP.length() +1);
      Serial.println(serverIP);
      Serial.println("Connect to tcp server");
      while (!client.connect(host, 8088, DELAY)) {
        Serial.println("connection failed");
      }
      // send client ip to server
      Serial.println("Connected");
      Serial.println("Send client IP");
      client.print(IDENTIFIER);
      client.stop();
      isConnectedServerTCP = true;
      return;
      }
  }else{
    Serial.println("Delay");
    delay(20000);
  }
}
