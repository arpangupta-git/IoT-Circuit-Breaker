#include <WiFi.h>
#include <WebServer.h>
#include "EmonLib.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- WiFi Credentials ---
const char* ssid = "Arpan Gupta_4G"; 
const char* password = "14032006";

// --- Pin Definitions ---
#define ZMPT_PIN 35
#define ACS_PIN 34
#define RELAY1_PIN 26
#define RELAY2_PIN 27
#define BUZZER_PIN 25

// --- Safety Thresholds ---
const float MAX_VOLTAGE = 250.0;
const float MIN_VOLTAGE = 180.0;
const float MAX_CURRENT = 5.0; 

// --- Objects ---
EnergyMonitor emon1; 
WebServer server(80);

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// --- Global Variables ---
float vRMS = 0.0;
float iRMS = 0.0;
float power = 0.0;
bool isTripped = false;
unsigned long lastSensorRead = 0;
unsigned long systemStartTime = 0; 
bool isSettling = true;            
String tripReason = "";            

// ========================================================================
// EMBEDDED HTML DASHBOARD
// ========================================================================
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="en"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0"><title>Circuit Breaker</title><script src="https://cdn.tailwindcss.com"></script><script src="https://unpkg.com/lucide@latest"></script><style>body{background-color:#0f172a;color:#f8fafc;font-family:sans-serif}.glass{background:rgba(30,41,59,0.7);border:1px solid rgba(255,255,255,0.1);border-radius:1.5rem;padding:1.5rem}</style></head><body class="p-6 max-w-4xl mx-auto"><h1 class="text-3xl font-bold mb-6 flex items-center gap-3"><i data-lucide="shield-zap" class="text-blue-400"></i> IoT Circuit Breaker Dashboard</h1><div class="grid grid-cols-1 md:grid-cols-3 gap-6 mb-6"><div class="glass"><p class="text-slate-400">Voltage</p><h2 class="text-4xl font-bold text-blue-400"><span id="v">0</span> V</h2></div><div class="glass"><p class="text-slate-400">Current</p><h2 class="text-4xl font-bold text-yellow-400"><span id="i">0</span> A</h2></div><div class="glass"><p class="text-slate-400">Power</p><h2 class="text-4xl font-bold text-purple-400"><span id="p">0</span> W</h2></div></div><div class="glass"><h3 class="text-xl font-semibold mb-4 text-green-400" id="status">Status: NORMAL</h3><div class="grid grid-cols-1 md:grid-cols-2 gap-4"><div class="p-4 bg-slate-800 rounded-lg border border-slate-700"><p class="font-bold mb-3">Bulb 1 (Main)</p><div class="flex gap-2"><button id="b1-on" onclick="toggle(1,'on')" class="bg-slate-700 px-4 py-2 rounded flex-1 transition-all">ON</button> <button id="b1-off" onclick="toggle(1,'off')" class="bg-slate-700 px-4 py-2 rounded flex-1 transition-all">OFF</button></div></div><div class="p-4 bg-slate-800 rounded-lg border border-slate-700"><p class="font-bold mb-3">Bulb 2 (Aux)</p><div class="flex gap-2"><button id="b2-on" onclick="toggle(2,'on')" class="bg-slate-700 px-4 py-2 rounded flex-1 transition-all">ON</button> <button id="b2-off" onclick="toggle(2,'off')" class="bg-slate-700 px-4 py-2 rounded flex-1 transition-all">OFF</button></div></div></div><div class="mt-6"><button onclick="trip()" class="bg-red-600 hover:bg-red-500 px-6 py-3 rounded font-bold w-full shadow-[0_0_15px_rgba(239,68,68,0.5)]">EMERGENCY TRIP</button></div></div><script>lucide.createIcons();setInterval(()=>{fetch('/api/data').then(r=>r.json()).then(d=>{document.getElementById('v').innerText=d.voltage.toFixed(1);document.getElementById('i').innerText=d.current.toFixed(2);document.getElementById('p').innerText=d.power.toFixed(0);const s=document.getElementById('status');s.innerText='Status: '+d.status;if(d.status.includes('FAULT')){s.className='text-xl font-semibold mb-4 text-red-500 animate-pulse'}else{s.className='text-xl font-semibold mb-4 text-green-400'} updateBtns('b1',d.relay1);updateBtns('b2',d.relay2);}).catch(e=>console.log(e))},1000);function updateBtns(id,isOn){const onBtn=document.getElementById(id+'-on');const offBtn=document.getElementById(id+'-off');if(isOn){onBtn.className='bg-blue-500 text-white px-4 py-2 rounded flex-1 shadow-[0_0_15px_rgba(59,130,246,0.6)] font-bold';offBtn.className='bg-slate-700 text-slate-400 px-4 py-2 rounded flex-1'}else{onBtn.className='bg-slate-700 text-slate-400 px-4 py-2 rounded flex-1';offBtn.className='bg-red-500 text-white px-4 py-2 rounded flex-1 shadow-[0_0_15px_rgba(239,68,68,0.6)] font-bold'}}function toggle(id,st){updateBtns('b'+id,st==='on');fetch(`/api/relay?id=${id}&state=${st}`)}function trip(){fetch('/api/trip')}</script></body></html>
)rawliteral";

// --- Functions ---

void updateOLED() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  
  display.setCursor(0,0);
  display.print("IoT Circuit Breaker");
  display.drawLine(0, 10, 128, 10, WHITE);

  display.setCursor(0, 15);
  display.print("V: "); display.print(vRMS, 1); display.print(" V");
  
  display.setCursor(0, 25);
  display.print("I: "); display.print(iRMS, 2); display.print(" A");
  
  display.setCursor(0, 35);
  display.print("P: "); display.print(power, 0); display.print(" W");

  display.setCursor(0, 50);
  if(isTripped) {
    display.setTextColor(BLACK, WHITE); 
    display.print(" TRIPPED! ");
  } else if (isSettling) {
    display.print(" Calibrating...  ");
  } else {
    display.print(" Status: NORMAL  ");
  }
  display.display();
}

void triggerFault(String reason) {
  isTripped = true;
  tripReason = reason;
  // Relays are usually Active LOW. High = Off.
  digitalWrite(RELAY1_PIN, HIGH); 
  digitalWrite(RELAY2_PIN, HIGH);
  // Turn Buzzer ON (Low-level trigger means LOW = ON)
  digitalWrite(BUZZER_PIN, LOW); 
  
  Serial.println("\n===========================");
  Serial.println("!!! FAULT DETECTED !!!");
  Serial.println("REASON: " + reason);
  Serial.print("Voltage at trip: "); Serial.println(vRMS);
  Serial.print("Current at trip: "); Serial.println(iRMS);
  Serial.println("===========================\n");
  
  updateOLED();
}

// --- API Endpoints ---
void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void handleData() {
  String json = "{";
  json += "\"voltage\":" + String(vRMS) + ",";
  json += "\"current\":" + String(iRMS) + ",";
  json += "\"power\":" + String(power) + ",";
  json += "\"relay1\":" + String(digitalRead(RELAY1_PIN) == LOW ? "true" : "false") + ",";
  json += "\"relay2\":" + String(digitalRead(RELAY2_PIN) == LOW ? "true" : "false") + ",";
  json += "\"status\":\"" + String(isTripped ? "FAULT: " + tripReason : "NORMAL") + "\"";
  json += "}";
  
  server.send(200, "application/json", json);
}

void handleRelay() {
  if (isTripped) {
    server.send(403, "text/plain", "System Tripped. Reset manually.");
    return;
  }
  
  String id = server.arg("id");
  String state = server.arg("state");
  
  int pin = (id == "1") ? RELAY1_PIN : RELAY2_PIN;
  int pinState = (state == "on") ? LOW : HIGH; 
  
  digitalWrite(pin, pinState);
  server.send(200, "text/plain", "OK");
}

void handleTrip() {
  triggerFault("Manual Web Trip");
  server.send(200, "text/plain", "TRIPPED");
}

void setup() {
  Serial.begin(115200);
  
  // FIX: Give the OLED screen and sensors 1 second to physically power up 
  // before the ESP32 tries to talk to them over the Hi-Link power supply.
  delay(1000); 
  
  analogReadResolution(10); 
  
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  // Start with relays ON (LOW = ON) and Buzzer OFF (HIGH = OFF for low-level trigger)
  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, LOW);
  digitalWrite(BUZZER_PIN, HIGH);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println("SSD1306 allocation failed");
  } else {
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setCursor(10, 15);
    display.println("Booting System...");
    display.setCursor(10, 35);
    display.println("Connecting Wi-Fi..."); 
    display.display();
  }

  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) { 
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected! IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nWiFi Connection Failed!");
    Serial.println("WARNING: ESP32 does not support 5GHz networks. Please use a 2.4GHz network.");
    
    display.clearDisplay();
    display.setCursor(0, 10);
    display.println("Wi-Fi Error!");
    display.setCursor(0, 30);
    display.println("Use 2.4GHz network");
    display.setCursor(0, 45);
    display.println("Not 5G!");
    display.display();
    delay(4000); 
  }

  // Calibrate Sensors 
  emon1.voltage(ZMPT_PIN, 234.26, 1.7); 
  emon1.current(ACS_PIN, 11.1);       

  server.on("/", handleRoot);
  server.on("/api/data", handleData);
  server.on("/api/relay", handleRelay);
  server.on("/api/trip", handleTrip);
  server.begin();

  systemStartTime = millis(); 
}

void loop() {
  server.handleClient();

  // Read sensors every 1 second
  if (millis() - lastSensorRead > 1000) {
    lastSensorRead = millis();

    if (!isTripped) {
      emon1.calcVI(20, 500); 
      
      vRMS = emon1.Vrms;
      iRMS = emon1.Irms;

      if (vRMS < 20.0) vRMS = 0.0;
      if (iRMS < 0.15) iRMS = 0.0;
      
      power = vRMS * iRMS;

      if (millis() - systemStartTime < 5000) {
        isSettling = true;
      } else {
        if (isSettling) {
            isSettling = false; 
            Serial.println("--- Calibration Complete. Monitoring Active ---");
        }
        
        if (vRMS > MAX_VOLTAGE) {
          triggerFault("Overvoltage");
        } else if (vRMS < MIN_VOLTAGE && vRMS > 100.0) { 
          triggerFault("Undervoltage");
        } else if (iRMS > MAX_CURRENT) {
          triggerFault("Overcurrent");
        }
      }
      
      updateOLED();
    }
  }
}