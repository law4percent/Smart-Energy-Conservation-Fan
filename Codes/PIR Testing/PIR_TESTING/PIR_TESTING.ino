// unsigned long previousMillis0 = 0;
// const long interval0 = 3000;  // 3 secs
// unsigned long previousMillis1 = 0;
// const long interval1 = 60000;  // 1 Min
// unsigned long previousMillis2 = 0;
// const long interval2 = 300000;  // 5 Mins

// bool Awake = true;
// bool pirMode = true;
// int countMotion = 0;

const byte PIR = 25;

void setup() {
  Serial.begin(115200);
  pinMode(PIR, INPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  int pirStatus = digitalRead(PIR);

  // unsigned long currentMillis = millis();

  Serial.print(F("PIR: "));
  Serial.println(pirStatus);

/*
  // 3 secs
  if (currentMillis - previousMillis0 >= interval0 and pirMode) {
    if (pirStatus > 500) {
      countMotion++;
    }
    previousMillis0 = currentMillis;
  }

  // 5 mins
  if (currentMillis - previousMillis2 >= interval2) {
    if (countMotion < 5 and pirMode) {
      // lastRotateData = RT_status;
      // AdjustSpeedFan(0);
      Awake = false;
      // UpdateFan();
    }
    previousMillis2 = currentMillis;
    countMotion = 0;
  }

  // Once the fan is asleep, the only task will be seeking motions
  if (!Awake and countMotion >= 5) {
    // CheckTempHumStatus();
    // RT_status = lastRotateData;
    Awake = true;
    // UpdateFan();
  }
*/
  delay(2000);
}
