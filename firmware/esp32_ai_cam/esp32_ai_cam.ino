/*
  ================================================================
  ESP32-AI-CAM / AI-Thinker ESP32-CAM
  Camera + Wi-Fi + UNO Event Monitor
  ================================================================

  This is the SECOND controller.

  It does NOT decide access.

  The Arduino UNO remains responsible for:
    - RFID
    - PIN
    - IR sensors
    - Load cell
    - Gate A / Gate B
    - Emergency
    - Fault logic

  This ESP32 sketch:
    - Initializes the OV2640 camera
    - Connects to Wi-Fi
    - Receives text events from the UNO over UART
    - Stores the latest event text
    - Takes an event photo for important alerts
    - Provides a small local web page
    - Provides /capture for a current photo
    - Provides /last-photo for the stored alert photo

  NO RTC.
  NO microSD.
  NO face recognition.
  NO AI / ML.
  NO remote gate-opening authority.

  ----------------------------------------------------------------
  FINAL UART WIRING
  ----------------------------------------------------------------

  ESP32 U0T / GPIO1 / TX  -> UNO A2 directly

  UNO A3 / TX
       -> CD4050 channel 5
       -> ESP32 U0R / GPIO3 / RX

  ESP32 GND -> common GND

  ----------------------------------------------------------------
  IMPORTANT PROGRAMMING NOTE
  ----------------------------------------------------------------

  ESP32-CAM normally uses GPIO1/GPIO3 for flashing too.

  If your programmer uses those pins:
    1. Disconnect UNO UART wires while uploading this sketch.
    2. Upload the ESP32 sketch.
    3. Boot normally.
    4. Reconnect UNO A2/A3 UART wires.

  ================================================================
*/


#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "esp_camera.h"


// ================================================================
// 1. WI-FI SETTINGS
// ================================================================
//
// CHANGE THESE.
//

const char *WIFI_SSID = "YOUR_WIFI_NAME";
const char *WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";


// ================================================================
// 2. AI-THINKER ESP32-CAM CAMERA PIN MAP
// ================================================================
//
// This mapping follows the AI-Thinker model used by the official
// Espressif CameraWebServer example.
//

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5

#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define FLASH_LED_GPIO     4


// ================================================================
// 3. WEB SERVER
// ================================================================

WebServer server(80);


// ================================================================
// 4. EVENT / PHOTO STORAGE
// ================================================================

String lastEvent = "BOOTING";

unsigned long lastEventTime = 0;

uint8_t *lastPhotoBuffer = NULL;
size_t lastPhotoLength = 0;


// ================================================================
// 5. UNO UART LINE BUFFER
// ================================================================
//
// UART0 is used at 9600 baud after boot.
//
// At runtime:
//   ESP32 TX GPIO1 -> UNO A2
//   ESP32 RX GPIO3 <- CD4050 <- UNO A3
//

char unoLineBuffer[96];
byte unoLineIndex = 0;


// ================================================================
// 6. FUNCTION PROTOTYPES
// ================================================================

bool initializeCamera();
void connectToWiFi();
void startWebServer();

void handleRootPage();
void handleStatus();
void handleCurrentCapture();
void handleLastPhoto();
void handleNotFound();

void processUnoSerial();
void processUnoEvent(const char *eventText);

bool eventNeedsPhoto(const char *eventText);
void captureAndStoreEventPhoto();
void clearStoredPhoto();

void sendJpegResponse(const uint8_t *buffer, size_t length);

String htmlEscape(const String &text);


// ================================================================
// 7. SETUP
// ================================================================

void setup()
{
    // This UART becomes the UNO communication link after boot.
    Serial.begin(9600);

    delay(800);

    pinMode(FLASH_LED_GPIO, OUTPUT);
    digitalWrite(FLASH_LED_GPIO, LOW);

    bool cameraOk = initializeCamera();

    connectToWiFi();

    if (cameraOk == true)
    {
        startWebServer();

        lastEvent = "ESP_READY";

        Serial.println("ESP_READY");

        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.print("ESP_IP:");
            Serial.println(WiFi.localIP());
        }
    }
    else
    {
        lastEvent = "CAMERA_FAULT";

        Serial.println("CAMERA_FAULT");
    }
}


// ================================================================
// 8. LOOP
// ================================================================

void loop()
{
    processUnoSerial();

    server.handleClient();

    // Simple Wi-Fi recovery.
    static unsigned long lastReconnectAttempt = 0;

    if (WiFi.status() != WL_CONNECTED)
    {
        if (millis() - lastReconnectAttempt > 10000)
        {
            lastReconnectAttempt = millis();

            WiFi.reconnect();
        }
    }

    delay(2);
}


// ================================================================
// 9. CAMERA INITIALIZATION
// ================================================================

bool initializeCamera()
{
    camera_config_t config = {};

    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;

    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;

    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;

    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;

    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;

    config.xclk_freq_hz = 20000000;

    // JPEG keeps web transfer and event-photo memory smaller.
    config.pixel_format = PIXFORMAT_JPEG;

    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;

    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location = CAMERA_FB_IN_DRAM;

    if (psramFound() == true)
    {
        config.frame_size = FRAMESIZE_VGA;
        config.jpeg_quality = 10;
        config.fb_count = 2;

        config.grab_mode = CAMERA_GRAB_LATEST;
        config.fb_location = CAMERA_FB_IN_PSRAM;
    }

    esp_err_t result = esp_camera_init(&config);

    if (result != ESP_OK)
    {
        return false;
    }

    // Lower initial resolution for a responsive demo.
    sensor_t *sensor = esp_camera_sensor_get();

    if (sensor != NULL)
    {
        sensor->set_framesize(sensor, FRAMESIZE_QVGA);
    }

    return true;
}


// ================================================================
// 10. WI-FI
// ================================================================

void connectToWiFi()
{
    WiFi.mode(WIFI_STA);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long startTime = millis();

    while (WiFi.status() != WL_CONNECTED)
    {
        if (millis() - startTime > 15000)
        {
            break;
        }

        delay(300);
    }
}


// ================================================================
// 11. WEB SERVER SETUP
// ================================================================

void startWebServer()
{
    server.on("/", HTTP_GET, handleRootPage);

    server.on("/status", HTTP_GET, handleStatus);

    server.on("/capture", HTTP_GET, handleCurrentCapture);

    server.on("/last-photo", HTTP_GET, handleLastPhoto);

    server.onNotFound(handleNotFound);

    server.begin();
}


// ================================================================
// 12. ROOT WEB PAGE
// ================================================================

void handleRootPage()
{
    String page;

    page.reserve(2500);

    page += "<!doctype html>";
    page += "<html>";
    page += "<head>";
    page += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
    page += "<meta http-equiv='refresh' content='4'>";
    page += "<title>Anti-Tailgating Gate</title>";

    page += "<style>";
    page += "body{font-family:Arial,sans-serif;max-width:760px;margin:30px auto;padding:0 16px;background:#f5f7fb;color:#18212f;}";
    page += ".card{background:white;border:1px solid #dbe2ec;border-radius:12px;padding:18px;margin:12px 0;}";
    page += "a{display:inline-block;padding:10px 14px;margin:4px;background:#2457c5;color:white;text-decoration:none;border-radius:8px;}";
    page += ".event{font-size:22px;font-weight:bold;}";
    page += "</style>";

    page += "</head>";
    page += "<body>";

    page += "<h1>Anti-Tailgating Security Gate</h1>";

    page += "<div class='card'>";
    page += "<b>ESP32-CAM status:</b> ";

    if (WiFi.status() == WL_CONNECTED)
    {
        page += "Wi-Fi connected";
    }
    else
    {
        page += "Wi-Fi disconnected";
    }

    page += "<br><br>";

    page += "<b>UNO last event:</b><br>";
    page += "<span class='event'>";
    page += htmlEscape(lastEvent);
    page += "</span>";

    page += "</div>";

    page += "<div class='card'>";
    page += "<a href='/capture'>Current Camera Image</a>";

    if (lastPhotoBuffer != NULL)
    {
        page += "<a href='/last-photo'>Last Alert Photo</a>";
    }

    page += "<a href='/status'>Plain Status</a>";
    page += "</div>";

    page += "<div class='card'>";
    page += "UNO remains the main safety controller. ";
    page += "The ESP32-CAM only handles camera, Wi-Fi and event monitoring.";
    page += "</div>";

    page += "</body>";
    page += "</html>";

    server.send(200, "text/html", page);
}


// ================================================================
// 13. STATUS ENDPOINT
// ================================================================

void handleStatus()
{
    String text;

    text = "Last event: ";
    text += lastEvent;
    text += "\n";

    if (WiFi.status() == WL_CONNECTED)
    {
        text += "WiFi: connected\n";
        text += "IP: ";
        text += WiFi.localIP().toString();
        text += "\n";
    }
    else
    {
        text += "WiFi: disconnected\n";
    }

    if (lastPhotoBuffer != NULL)
    {
        text += "Stored alert photo: yes\n";
    }
    else
    {
        text += "Stored alert photo: no\n";
    }

    server.send(200, "text/plain", text);
}


// ================================================================
// 14. CURRENT CAMERA CAPTURE
// ================================================================

void handleCurrentCapture()
{
    camera_fb_t *frame = esp_camera_fb_get();

    if (frame == NULL)
    {
        server.send(500, "text/plain", "Camera capture failed.");

        return;
    }

    sendJpegResponse(frame->buf, frame->len);

    esp_camera_fb_return(frame);
}


// ================================================================
// 15. LAST EVENT PHOTO
// ================================================================

void handleLastPhoto()
{
    if (lastPhotoBuffer == NULL)
    {
        server.send(404, "text/plain", "No alert photo has been stored yet.");

        return;
    }

    sendJpegResponse(lastPhotoBuffer, lastPhotoLength);
}


// ================================================================
// 16. BINARY JPEG RESPONSE
// ================================================================

void sendJpegResponse(const uint8_t *buffer, size_t length)
{
    server.setContentLength(length);

    server.send(200, "image/jpeg", "");

    WiFiClient client = server.client();

    client.write(buffer, length);
}


// ================================================================
// 17. 404
// ================================================================

void handleNotFound()
{
    server.send(404, "text/plain", "Not found.");
}


// ================================================================
// 18. READ UNO EVENT LINES
// ================================================================

void processUnoSerial()
{
    while (Serial.available() > 0)
    {
        char incoming = Serial.read();

        if (incoming == '\r')
        {
            continue;
        }

        if (incoming == '\n')
        {
            unoLineBuffer[unoLineIndex] = '\0';

            if (unoLineIndex > 0)
            {
                processUnoEvent(unoLineBuffer);
            }

            unoLineIndex = 0;

            continue;
        }

        if (unoLineIndex < sizeof(unoLineBuffer) - 1)
        {
            unoLineBuffer[unoLineIndex] = incoming;

            unoLineIndex++;
        }
        else
        {
            // Protect against an unexpectedly long UART line.
            unoLineIndex = 0;
        }
    }
}


// ================================================================
// 19. PROCESS UNO EVENT
// ================================================================

void processUnoEvent(const char *eventText)
{
    lastEvent = eventText;

    lastEventTime = millis();

    if (eventNeedsPhoto(eventText) == true)
    {
        captureAndStoreEventPhoto();
    }

    /*
      --------------------------------------------------------------
      OPTIONAL LATER FEATURE: TELEGRAM / INTERNET NOTIFICATION
      --------------------------------------------------------------

      You can later add a function here such as:

          sendTelegramNotification(eventText);

      Recommended events:
          AUTH_DENIED
          POSSIBLE_TAILGATING
          FAULT
          EMERGENCY

      Keep it commented / absent until the local camera + web page
      works reliably.

      Telegram must NEVER be allowed to command Gate B open.
      --------------------------------------------------------------
    */
}


// ================================================================
// 20. WHICH EVENTS TAKE A PHOTO?
// ================================================================

bool eventNeedsPhoto(const char *eventText)
{
    if (strcmp(eventText, "AUTH_DENIED") == 0)
    {
        return true;
    }

    if (strcmp(eventText, "POSSIBLE_TAILGATING") == 0)
    {
        return true;
    }

    if (strcmp(eventText, "EMERGENCY") == 0)
    {
        return true;
    }

    // UNO currently sends plain "FAULT".
    if (strncmp(eventText, "FAULT", 5) == 0)
    {
        return true;
    }

    return false;
}


// ================================================================
// 21. STORE ALERT PHOTO IN RAM
// ================================================================
//
// We do not use microSD.
//
// Therefore the most recent important-event photo is copied into RAM.
// A new alert replaces the previous photo.
//
// QVGA JPEG is used after startup to keep image size manageable.
//

void captureAndStoreEventPhoto()
{
    camera_fb_t *frame = esp_camera_fb_get();

    if (frame == NULL)
    {
        return;
    }

    clearStoredPhoto();

    lastPhotoBuffer = (uint8_t *)malloc(frame->len);

    if (lastPhotoBuffer != NULL)
    {
        memcpy(lastPhotoBuffer, frame->buf, frame->len);

        lastPhotoLength = frame->len;
    }

    esp_camera_fb_return(frame);
}


// ================================================================
// 22. FREE PREVIOUS PHOTO
// ================================================================

void clearStoredPhoto()
{
    if (lastPhotoBuffer != NULL)
    {
        free(lastPhotoBuffer);

        lastPhotoBuffer = NULL;
        lastPhotoLength = 0;
    }
}


// ================================================================
// 23. SMALL HTML ESCAPE HELPER
// ================================================================

String htmlEscape(const String &text)
{
    String result;

    result.reserve(text.length() + 16);

    for (size_t i = 0; i < text.length(); i++)
    {
        char c = text.charAt(i);

        if (c == '&')
        {
            result += "&amp;";
        }
        else if (c == '<')
        {
            result += "&lt;";
        }
        else if (c == '>')
        {
            result += "&gt;";
        }
        else
        {
            result += c;
        }
    }

    return result;
}


/*
  ================================================================
  OPTIONAL / LATER ESP32 FEATURES
  ================================================================

  Keep these OUT until the basic system works:

  1. Telegram Bot
  2. Cloud dashboard
  3. Remote event database
  4. Continuous video streaming
  5. Face recognition / AI / ML

  The current local page + still image + alert snapshot is enough
  to prove that the ESP32-CAM is integrated.

  Most importantly:
    ESP32 must never bypass the UNO interlock.
  ================================================================
*/
