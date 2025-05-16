#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED Display Settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// WiFi Credentials
const char* ssid = "Your_WiFi_SSID";
const char* password = "Your_WiFi_Password";

// Web Server
WebServer server(80);

// Current message
String currentMessage = "Ready for messages...";
bool newMessageAvailable = false;

void setup() {
  // Initialize Serial
  Serial.begin(115200);
  
  // Initialize OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.display();
  delay(2000);
  display.clearDisplay();
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Connecting to WiFi...");
  display.display();
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  // Display connection info
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("WiFi connected!");
  display.print("IP: ");
  display.println(WiFi.localIP());
  display.display();
  delay(2000);
  
  // Web Server Routes
  server.on("/", handleRoot);
  server.on("/message", handleMessage);
  server.on("/ping", handlePing);
  
  // Start server
  server.begin();
  Serial.println("HTTP server started");
  
  // Show initial message
  updateDisplay();
}

void loop() {
  server.handleClient();
  
  if (newMessageAvailable) {
    updateDisplay();
    newMessageAvailable = false;
  }
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><title>ESP32 Message Receiver</title></head>";
  html += "<body><h1>ESP32 Message Receiver</h1>";
  html += "<p>Current message: " + currentMessage + "</p>";
  html += "<p>Send messages to /message?text=your_message</p>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handleMessage() {
  if (server.hasArg("text")) {
    currentMessage = server.arg("text");
    newMessageAvailable = true;
    
    // Log to serial
    Serial.print("New message received: ");
    Serial.println(currentMessage);
    
    server.send(200, "text/plain", "Message received: " + currentMessage);
  } else {
    server.send(400, "text/plain", "Bad Request: Missing 'text' parameter");
  }
}

void handlePing() {
  server.send(200, "text/plain", "OK");
}

void updateDisplay() {
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  
  // Display WiFi status
  display.print("IP: ");
  display.println(WiFi.localIP());
  display.println("----------------");
  
  // Display message with word wrapping
  display.setTextSize(2);
  int16_t x = 0, y = 16;
  uint16_t w, h;
  
  // Split message into lines that fit the display
  String remainingText = currentMessage;
  while (remainingText.length() > 0) {
    // Find how many characters fit on this line
    int charsToPrint = 0;
    String testString;
    do {
      charsToPrint++;
      testString = remainingText.substring(0, charsToPrint);
      display.getTextBounds(testString, 0, 0, &x, &y, &w, &h);
    } while (w < SCREEN_WIDTH && charsToPrint < remainingText.length());
    
    // If we went past the end, back up to last space
    if (w >= SCREEN_WIDTH) {
      int lastSpace = testString.lastIndexOf(' ');
      if (lastSpace > 0) {
        charsToPrint = lastSpace;
      } else {
        charsToPrint--; // Just print what fits
      }
    }
    
    // Print this line
    display.println(remainingText.substring(0, charsToPrint));
    remainingText = remainingText.substring(charsToPrint);
    
    // Stop if we're out of vertical space
    if (display.getCursorY() > SCREEN_HEIGHT - 16) {
      break;
    }
  }
  
  display.display();
}