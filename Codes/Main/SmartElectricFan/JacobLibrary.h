// Pin Used
const byte DHTPIN = 26;
const byte PIR = 25;
const byte Speed1 = 13;
const byte Speed2 = 12;
const byte Speed3 = 14;
const byte Rotate = 27;
const byte LEDindecatorMode = 18;
const byte ReadyLEDIndicator = 19;

float hum = 0.0;
float temp = 0.0;
bool pirStatus;
int countMotion = 0;
int Index = 0;
bool Spd1_status = false;
bool Spd2_status = false;
bool Spd3_status = false;
bool RT_status = false;
bool manualMode = false;
bool Awake = true;
bool pirMode = true;
bool dhtStatus = false;
bool lastRotateData;
bool ShutdownFan = false;

unsigned long previousMillis1 = 0;
const long interval_1min = 60000;  // 1 Min
unsigned long previousMillis = 0;
const long TimeInterval = 30000;

byte last_speed;
const int delay5sec = 3000;

bool startSleep = true;
unsigned long StartToCount_Sleep;
unsigned long EndSleep;
