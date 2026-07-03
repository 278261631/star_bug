void setup() {
  Serial.begin(115200);
  for (int i = 0; i < 10; i++) {
    Serial.println("Hello from ESP32-S3!");
    delay(500);
  }
}

void loop() {
  Serial.println("Loop running...");
  delay(2000);
}
