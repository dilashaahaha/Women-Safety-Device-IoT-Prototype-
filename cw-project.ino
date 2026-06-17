#include <HardwareSerial.h>
#include <TinyGPSPlus.h>

//OBJECTS
HardwareSerial sim800(2);     // UART2 → SIM800L
HardwareSerial gpsSerial(1);  // UART1 → GPS
TinyGPSPlus gps;

//  SETTINGS
const String PHONE_NUMBER = "+9779745969067";

const int switchPin = 4;
const int ledPin = 5;
int switchVal;
bool smsSent = false;

// SETUP
void setup() {
  Serial.begin(115200);

  // SIM800L: RX=16 TX=17
  sim800.begin(9600, SERIAL_8N1, 16, 17);

  // GPS: RX=27 TX=26
  gpsSerial.begin(9600, SERIAL_8N1, 27, 26);
  pinMode(switchPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);


  Serial.println("Initializing SIM800L...");
  delay(3000);

  sendAT("AT");
  sendAT("AT+CSQ");     // Signal quality
  sendAT("AT+CREG?");   // Network registration
  sendAT("AT+CMGF=1");  // SMS text mode

  Serial.println("Waiting for GPS fix...");
}

// ---------- LOOP ----------
void loop() {
  // Read GPS data
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  // Show GPS info
  if (gps.location.isValid()) {
    Serial.print("Lat: ");
    Serial.println(gps.location.lat(), 6);
    Serial.print("Lng: ");
    Serial.println(gps.location.lng(), 6);
  } else {
    Serial.println("No GPS fix yet...");
  }
  //Read when switch is pressed
  switchVal = digitalRead(switchPin);
  Serial.println(switchVal);
  // Send SMS once GPS + Network are ready
  if (switchVal == 0 && !smsSent) {

    if (gps.location.isValid() && isNetworkRegistered()) {

      float lat = gps.location.lat();
      float lng = gps.location.lng();

      String message = "Help Its urgent\n";
      message += "Location Alert\n";
      message += "Lat: " + String(lat, 6) + "\n";
      message += "Lng: " + String(lng, 6) + "\n";
      message += "Map: https://maps.google.com/?q=";
      message += String(lat, 6) + "," + String(lng, 6);

      sendSMS(PHONE_NUMBER, message);
      digitalWrite(ledPin, HIGH);
      delay(2000);
      digitalWrite(ledPin, LOW);
      smsSent = true;
      delay(1000);
    }
  } else {
      smsSent = false;
       digitalWrite(ledPin, LOW);
  }

  //delay(2000);
}

// ---------- FUNCTIONS ----------

// Send SMS
void sendSMS(String number, String message) {
  Serial.println("Sending SMS...");

  sim800.println("AT+CMGF=1");
  delay(500);

  sim800.print("AT+CMGS=\"");
  sim800.print(number);
  sim800.println("\"");

  if (!waitForPrompt('>', 10000)) {
    Serial.println("❌ Failed to get '>' prompt");
    return;
  }

  sim800.print(message);
  sim800.write(26);  // CTRL+Z

  Serial.println("✅ SMS Sent");
}

// Check network registration
bool isNetworkRegistered() {
  sim800.println("AT+CREG?");
  delay(500);

  String response = "";
  while (sim800.available()) {
    response += char(sim800.read());
  }

  Serial.println("CREG: " + response);

  return (response.indexOf(",1") != -1 || response.indexOf(",5") != -1);
}

// Wait for specific character (>, OK, etc.)
bool waitForPrompt(char prompt, unsigned long timeout) {
  unsigned long start = millis();
  while (millis() - start < timeout) {
    while (sim800.available()) {
      char c = sim800.read();
      Serial.write(c);
      if (c == prompt) return true;
    }
  }
  return false;
}

// Simple AT command sender
void sendAT(const char *cmd) {
  Serial.print(">> ");
  Serial.println(cmd);
  sim800.println(cmd);
  delay(800);
  while (sim800.available()) {
    Serial.write(sim800.read());
  }
  Serial.println();
}