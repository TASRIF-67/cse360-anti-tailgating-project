/*
  TEST 13 - ESP32-AI-CAM CAMERA + WI-FI

  Purpose:
  Verify the camera and Wi-Fi independently before integrating UNO.

  This is for the common AI-Thinker ESP32-CAM pin mapping.

  Change:
    WIFI_SSID
    WIFI_PASSWORD

  After boot, open the printed IP address in a browser.
  The root page shows a link to /capture.
*/

#include <WiFi.h>
#include <WebServer.h>
#include "esp_camera.h"


const char *WIFI_SSID = "YOUR_WIFI_NAME";
const char *WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";


// AI-Thinker ESP32-CAM pin mapping.
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


WebServer server(80);


bool initializeCamera();
void handleRoot();
void handleCapture();


void setup()
{
    Serial.begin(115200);

    delay(800);

    if (initializeCamera() == false)
    {
        Serial.println("Camera initialization failed.");

        while (true)
        {
            delay(1000);
        }
    }

    WiFi.mode(WIFI_STA);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting to Wi-Fi");

    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print('.');

        delay(500);
    }

    Serial.println();
    Serial.println("Wi-Fi connected.");

    Serial.print("Open: http://");
    Serial.println(WiFi.localIP());

    server.on("/", HTTP_GET, handleRoot);
    server.on("/capture", HTTP_GET, handleCapture);

    server.begin();
}


void loop()
{
    server.handleClient();

    delay(2);
}


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

    return true;
}


void handleRoot()
{
    String page;

    page = "<!doctype html>";
    page += "<html><body>";
    page += "<h1>ESP32-CAM Test</h1>";
    page += "<p><a href='/capture'>Take current photo</a></p>";
    page += "</body></html>";

    server.send(200, "text/html", page);
}


void handleCapture()
{
    camera_fb_t *frame = esp_camera_fb_get();

    if (frame == NULL)
    {
        server.send(500, "text/plain", "Camera capture failed.");

        return;
    }

    server.setContentLength(frame->len);

    server.send(200, "image/jpeg", "");

    WiFiClient client = server.client();

    client.write(frame->buf, frame->len);

    esp_camera_fb_return(frame);
}
