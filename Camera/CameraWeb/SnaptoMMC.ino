#define CAMERA_MODEL_AI_THINKER 
#include "esp_camera.h" 
#include "camera_pins.h" 
#include "FS.h" 
#include "SD_MMC.h" 
#include "EEPROM.h" 
#include "WiFi.h" 
#include "ESPAsyncWebServer.h" 

#define EEPROM_SIZE 1
// const char *ssid = "9ngo12T3";
// const char *password = "55556666";
const char *ssid = "VNPT_Thinh Hue";
const char *password = "12345678";
// const char *ssid = "ESP";
// const char *password = "12345678d";

// Tạo đối tượng AsyncWebServer trên cổng 80
AsyncWebServer server(80);
int pictureNumber = 0;
bool isCapturing = false; // Trạng thái chụp ảnh liên tục
unsigned long lastCaptureTime = 0;
const int captureInterval = 100; // 1 giây giữa các lần chụp ảnh

// Hàm phụ trợ để lấy danh sách tệp từ thẻ SD
String listFiles() {
  String fileList = "";
  File root = SD_MMC.open("/");
  if (!root) {
    return "Không thể mở thư mục gốc";
  }
  
  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      String fileName = String(file.name());
      if (fileName.endsWith(".jpg")) {
        fileList += "<li>📷 <a href=\"/picture?name=";
        fileList += fileName;
        fileList += "\">";
        fileList += fileName;
        fileList += "</a> (";
        fileList += String(file.size());
        fileList += " bytes) <a href=\"/download?name=";
        fileList += fileName;
        fileList += "\">⬇️ Tải</a> <a href=\"/delete?name=";
        fileList += fileName;
        fileList += "\">❌ Xóa</a></li>";
      }
    }
    file = root.openNextFile();
  }
  return fileList;
}
// Trang HTML chính
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <meta charset="utf-8">
  <title>ESP32-CAM Chụp ảnh liên tục</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      font-family: Arial, sans-serif;
      margin: 20px;
      text-align: center;
      background-color: #f0f0f0;
    }
    .button {
      border: none;
      color: white;
      padding: 20px 40px;
      text-align: center;
      text-decoration: none;
      display: inline-block;
      font-size: 18px;
      margin: 10px 5px;
      cursor: pointer;
      border-radius: 12px;
      box-shadow: 0 6px 12px rgba(0,0,0,0.3);
      transition: all 0.3s;
      font-weight: bold;
    }
    .button:hover {
      transform: translateY(-3px);
      box-shadow: 0 8px 16px rgba(0,0,0,0.4);
    }
    .button.start {
      background-color: #4CAF50;
    }
    .button.start:hover {
      background-color: #45a049;
    }
    .button.stop {
      background-color: #f44336;
    }
    .button.stop:hover {
      background-color: #da190b;
    }
    .button.single {
      background-color: #2196F3;
    }
    .button.single:hover {
      background-color: #1976D2;
    }
    h1 {
      color: #0066cc;
      text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
      margin-bottom: 10px;
    }
    .container {
      max-width: 900px;
      margin: 0 auto;
      background-color: white;
      padding: 30px;
      border-radius: 15px;
      box-shadow: 0 6px 15px rgba(0,0,0,0.1);
    }
    ul {
      text-align: left;
      list-style-type: none;
      padding: 0;
      max-height: 400px;
      overflow-y: auto;
    }
    li {
      margin-bottom: 12px;
      padding: 15px;
      background-color: #f9f9f9;
      border-radius: 8px;
      border-left: 5px solid #4CAF50;
      box-shadow: 0 2px 4px rgba(0,0,0,0.1);
    }
    .status {
      font-weight: bold;
      padding: 20px;
      border-radius: 10px;
      margin: 20px 0;
      font-size: 18px;
    }
    .capturing {
      background-color: #e8f5e8;
      color: #2e7d32;
      border: 3px solid #4caf50;
      animation: pulse 2s infinite;
    }
    .stopped {
      background-color: #f5f5f5;
      color: #666;
      border: 3px solid #ccc;
    }
    @keyframes pulse {
      0% { transform: scale(1); }
      50% { transform: scale(1.02); }
      100% { transform: scale(1); }
    }
    .controls {
      margin: 30px 0;
      padding: 25px;
      background-color: #fafafa;
      border-radius: 15px;
      border: 2px solid #e0e0e0;
    }
    .controls h2 {
      margin-top: 0;
      color: #333;
      font-size: 24px;
    }
    .info {
      background-color: #e3f2fd;
      padding: 15px;
      border-radius: 8px;
      margin: 20px 0;
      border-left: 5px solid #2196f3;
    }
    .counter {
      font-size: 20px;
      font-weight: bold;
      color: #2e7d32;
      margin: 15px 0;
    }
  </style>
  <script>
    function updateStatus() {
      fetch('/status')
        .then(response => response.text())
        .then(data => {
          document.getElementById('status').innerHTML = data;
        });
    }
    
    function updateFileList() {
      fetch('/filelist')
        .then(response => response.text())
        .then(data => {
          document.getElementById('fileList').innerHTML = data;
        });
    }
    
    setInterval(updateStatus, 1000); // Cập nhật trạng thái mỗi giây
    setInterval(updateFileList, 2000); // Cập nhật danh sách file mỗi 2 giây

    function startCapture() {
      fetch('/startCapture')
        .then(() => {
          updateStatus();
          setTimeout(updateFileList, 1000);
        });
    }

    function stopCapture() {
      fetch('/stopCapture')
        .then(() => {
          updateStatus();
          setTimeout(updateFileList, 1000);
        });
    }

    function singleCapture() {
      fetch('/singleCapture')
        .then(() => {
          setTimeout(updateFileList, 1000);
        });
    }
  </script>
</head>
<body>
  <div class="container">
    <h1>📷 ESP32-CAM Chụp ảnh liên tục</h1>
    
    <div id="status" class="status">%STATUS%</div>
    
    <div class="controls">
      <h2>🎯 Điều khiển chụp ảnh</h2>
      <button class="button start" onclick="startCapture()">▶️ Bắt đầu chụp liên tục</button>
      <button class="button stop" onclick="stopCapture()">⏹️ Dừng chụp liên tục</button>
      <button class="button single" onclick="singleCapture()">📸 Chụp 1 ảnh</button>
      
      <div class="info">
        <strong>📝 Hướng dẫn:</strong><br>
        • <strong>Chụp liên tục:</strong> Chụp ảnh tự động mỗi giây cho đến khi nhấn dừng<br>
        • <strong>Chụp 1 ảnh:</strong> Chụp ngay 1 ảnh duy nhất<br>
        • Tất cả ảnh được lưu vào thẻ SD với tên tự động
      </div>
    </div>
    
    <h2>📁 Danh sách ảnh trên thẻ SD: <span id="photoCount" class="counter">%PHOTO_COUNT%</span></h2>      
    <ul id="fileList">%FILES%</ul>
  </div>
</body>
</html>
)rawliteral";

void startCameraServer() {
  // Định tuyến cho trang chủ
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String files = listFiles();
    String html = String(index_html);
    html.replace("%FILES%", files);
    
    // Đếm số ảnh
    int photoCount = 0;
    File root = SD_MMC.open("/");
    if (root) {
      File file = root.openNextFile();
      while (file) {
        if (!file.isDirectory() && String(file.name()).endsWith(".jpg")) {
          photoCount++;
        }
        file = root.openNextFile();
      }
    }
    html.replace("%PHOTO_COUNT%", String(photoCount) + " ảnh");
    
    String status = "";
    if (isCapturing) {
      status = "<div class='capturing'>📷 ĐANG CHỤP ẢNH LIÊN TỤC... (mỗi giây)</div>";
    } else {
      status = "<div class='stopped'>⏸️ Đã dừng chụp ảnh</div>";
    }
    html.replace("%STATUS%", status);
    
    request->send(200, "text/html", html);
  });

  // Định tuyến để lấy trạng thái
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
    String status = "";
    if (isCapturing) {
      status = "<div class='capturing'>📷 ĐANG CHỤP ẢNH LIÊN TỤC... (mỗi giây)</div>";
    } else {
      status = "<div class='stopped'>⏸️ Đã dừng chụp ảnh</div>";
    }
    request->send(200, "text/html", status);
  });

  // Định tuyến để lấy danh sách file
  server.on("/filelist", HTTP_GET, [](AsyncWebServerRequest *request){
    String files = listFiles();
    request->send(200, "text/html", files);
  });

  // Định tuyến để bắt đầu chụp ảnh liên tục
  server.on("/startCapture", HTTP_GET, [](AsyncWebServerRequest *request){
    if (!isCapturing) {
      isCapturing = true;
      lastCaptureTime = millis();
      Serial.println("Bắt đầu chụp ảnh liên tục");
    }
    request->send(200, "text/plain", "OK");
  });

  // Định tuyến để dừng chụp ảnh liên tục
  server.on("/stopCapture", HTTP_GET, [](AsyncWebServerRequest *request){
    if (isCapturing) {
      isCapturing = false;
      Serial.println("Dừng chụp ảnh liên tục");
    }
    request->send(200, "text/plain", "OK");
  });

  // Định tuyến để chụp 1 ảnh đơn lẻ
  server.on("/singleCapture", HTTP_GET, [](AsyncWebServerRequest *request){
    camera_fb_t * fb = esp_camera_fb_get();  
    if(!fb) {
      request->send(500, "text/plain", "Lỗi chụp ảnh");
      return;
    }
    
    pictureNumber = EEPROM.read(0) + 1;
    String path = "/picture" + String(pictureNumber) + ".jpg";
    
    File file = SD_MMC.open(path.c_str(), FILE_WRITE);
    if(!file) {
      request->send(500, "text/plain", "Lỗi mở tệp để ghi");
    } 
    else {
      file.write(fb->buf, fb->len);
      EEPROM.write(0, pictureNumber);
      EEPROM.commit();
      file.close();
      Serial.println("Đã chụp ảnh đơn lẻ: " + path);
      request->send(200, "text/plain", "OK");
    }
    
    esp_camera_fb_return(fb);
  });

  // Định tuyến để xem ảnh
  server.on("/picture", HTTP_GET, [](AsyncWebServerRequest *request){
    if(!request->hasParam("name")) {
      request->send(400, "text/plain", "Thiếu tham số name");
      return;
    }
    
    String fileName = request->getParam("name")->value();
    if (!SD_MMC.exists("/" + fileName)) {
      request->send(404, "text/plain", "Không tìm thấy tệp");
      return;
    }
    
    request->send(SD_MMC, "/" + fileName, "image/jpeg");
  });

  // Định tuyến để tải xuống
  server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request){
    if(!request->hasParam("name")) {
      request->send(400, "text/plain", "Thiếu tham số name");
      return;
    }
    
    String fileName = request->getParam("name")->value();
    if (!SD_MMC.exists("/" + fileName)) {
      request->send(404, "text/plain", "Không tìm thấy tệp");
      return;
    }
    
    AsyncWebServerResponse *response = request->beginResponse(SD_MMC, "/" + fileName, "image/jpeg", true);
    request->send(response);
  });

  // Định tuyến để xóa tệp
  server.on("/delete", HTTP_GET, [](AsyncWebServerRequest *request){
    if(!request->hasParam("name")) {
      request->send(400, "text/plain", "Thiếu tham số name");
      return;
    }
    
    String fileName = request->getParam("name")->value();
    if (!SD_MMC.exists("/" + fileName)) {
      request->send(404, "text/plain", "Không tìm thấy tệp");
      return;
    }
    
    SD_MMC.remove("/" + fileName);
    request->redirect("/");
  });

  server.begin();
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  // Cấu hình IP tĩnh
  // IPAddress STATIC_IP(192,168,200,113);
  // IPAddress STATIC_IP(192,168,207,10);
  // if(!WiFi.config(STATIC_IP)){
  //   Serial.println("Cấu hình IP tĩnh thất bại");
  // }

  // Cấu hình camera
  static camera_config_t config; 
  config.ledc_channel       = LEDC_CHANNEL_0;
  config.ledc_timer         = LEDC_TIMER_0;
  config.pin_d0             = Y2_GPIO_NUM;
  config.pin_d1             = Y3_GPIO_NUM;
  config.pin_d2             = Y4_GPIO_NUM;
  config.pin_d3             = Y5_GPIO_NUM;
  config.pin_d4             = Y6_GPIO_NUM;
  config.pin_d5             = Y7_GPIO_NUM;
  config.pin_d6             = Y8_GPIO_NUM;
  config.pin_d7             = Y9_GPIO_NUM;
  config.pin_xclk           = XCLK_GPIO_NUM;
  config.pin_pclk           = PCLK_GPIO_NUM;
  config.pin_vsync          = VSYNC_GPIO_NUM;
  config.pin_href           = HREF_GPIO_NUM;
  config.pin_sscb_sda       = SIOD_GPIO_NUM;
  config.pin_sscb_scl       = SIOC_GPIO_NUM;
  config.pin_pwdn           = PWDN_GPIO_NUM;
  config.pin_reset          = RESET_GPIO_NUM;
  config.xclk_freq_hz       = 20000000;
  config.pixel_format       = PIXFORMAT_JPEG;
  config.frame_size         = FRAMESIZE_UXGA; // Độ phân giải cao cho ảnh
  config.jpeg_quality       = 10; // Chất lượng cao cho ảnh
  config.fb_count           = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Khởi tạo camera thất bại với lỗi 0x%x", err);
    return;
  }
  
  // Cài đặt camera
  sensor_t * s = esp_camera_sensor_get();
  s->set_brightness(s, 0);
  s->set_contrast(s, 0);
  s->set_saturation(s, 0);
  s->set_special_effect(s, 0);
  s->set_whitebal(s, 1);
  s->set_awb_gain(s, 1);
  s->set_wb_mode(s, 0);
  s->set_exposure_ctrl(s, 1);
  s->set_aec2(s, 1);
  s->set_ae_level(s, 0);
  s->set_aec_value(s, 1200);
  s->set_gain_ctrl(s, 1);
  s->set_agc_gain(s, 30);
  s->set_gainceiling(s, (gainceiling_t)6);
  s->set_bpc(s, 1);
  s->set_wpc(s, 1);
  s->set_raw_gma(s, 1);
  s->set_lenc(s, 1);
  s->set_hmirror(s, 1);
  s->set_vflip(s, 1);
  s->set_dcw(s, 1);
  s->set_colorbar(s, 0);

  // Khởi tạo thẻ SD
  if(!SD_MMC.begin()){
    Serial.println("Gắn thẻ SD thất bại");
    return;
  }
  
  uint8_t cardType = SD_MMC.cardType();
  if(cardType == CARD_NONE){
    Serial.println("Không có thẻ SD được gắn");
    return;
  }
  
  // Khởi tạo EEPROM
  EEPROM.begin(EEPROM_SIZE);
  pictureNumber = EEPROM.read(0);

  // Kết nối WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Đang kết nối WiFi...");
  }

  Serial.println("WiFi đã kết nối");
  Serial.print("Địa chỉ IP: ");
  Serial.println(WiFi.localIP());

  startCameraServer();
  Serial.println("Máy chủ web đã bắt đầu");
}

void loop() {
  unsigned long currentTime = millis();
  
  // Xử lý chụp ảnh liên tục
  if (isCapturing && currentTime - lastCaptureTime >= captureInterval) {
    camera_fb_t * fb = esp_camera_fb_get();
    if (fb) {
      pictureNumber = EEPROM.read(0) + 1;
      String path = "/picture" + String(pictureNumber) + ".jpg";
      
      File file = SD_MMC.open(path.c_str(), FILE_WRITE);
      if (file) {
        file.write(fb->buf, fb->len);
        EEPROM.write(0, pictureNumber);
        EEPROM.commit();
        file.close();
        Serial.println("Đã chụp ảnh: " + path);
      }
      
      esp_camera_fb_return(fb);
      lastCaptureTime = currentTime;
    }
  }
  
  delay(10);
}