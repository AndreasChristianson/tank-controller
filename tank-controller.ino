// #include <lvgl.h>
#include <LilyGo_AMOLED.h>
#include <TFT_eSPI.h>
#include <esp_sleep.h>
#include <ArduinoBLE.h>

BLEService tankService("E6B81F14-F9E5-40C9-A739-4DE4564264D1");
BLEIntCharacteristic lxChar("6AB4", BLERead | BLENotify|BLEWrite);
BLEIntCharacteristic lyChar("A096", BLERead | BLENotify|BLEWrite);
BLEBoolCharacteristic lwChar("11D2", BLERead | BLENotify|BLEWrite);
BLEIntCharacteristic rxChar("87B4", BLERead | BLENotify|BLEWrite);
BLEIntCharacteristic ryChar("B5EC", BLERead | BLENotify|BLEWrite);
BLEBoolCharacteristic rwChar("3A8E", BLERead | BLENotify|BLEWrite);


#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);
LilyGo_Class amoled;

#define WIDTH amoled.height()
#define HEIGHT amoled.width()

int ryLast = 1900;
int rxLast = 1900;
bool rwLast = false;
int lyLast = 1900;
int lxLast = 1900;
bool lwLast = false;
void setup() {

  Serial.begin(9600);

  if (!BLE.begin()) {
    Serial.println("starting Bluetooth Low Energy module failed!");

    while (1)
      ;
  }

  tankService.addCharacteristic(lxChar);
  tankService.addCharacteristic(lyChar);
  tankService.addCharacteristic(lwChar);
  tankService.addCharacteristic(rxChar);
  tankService.addCharacteristic(ryChar);
  tankService.addCharacteristic(rwChar);

  BLE.setLocalName("tank controller");
  BLE.addService(tankService);
  BLE.setAdvertisedService(tankService);




  // set the initial value for the characeristic:
  lxChar.writeValue(lxLast);
  lyChar.writeValue(lyLast);
  lwChar.writeValue(lwLast);
  rxChar.writeValue(rxLast);
  ryChar.writeValue(ryLast);
  rwChar.writeValue(rwLast);

  BLE.advertise();
  if (!amoled.begin()) {
    while (1) {
      Serial.println("There is a problem with the device!~");
      delay(1000);
    }
  }

  tft.setRotation(3);

  spr.createSprite(WIDTH, HEIGHT);
  Serial.println("Controller setup complete");
  spr.fillSprite(TFT_BLACK);

  spr.setTextSize(5);

  spr.setTextColor(TFT_ORANGE);
  spr.drawString("Welcome!", 0, 0);
  amoled.pushColors(0, 0, WIDTH, HEIGHT, (uint16_t*)spr.getPointer());
  delay(1000);
}


bool detectChange(int* previous, int current, BLEIntCharacteristic characteristic) {
  if (abs(*previous - current) > 100) {
    Serial.println("change found!");
    Serial.print(*previous);
    Serial.print(" != ");
    Serial.println(current);
    characteristic.writeValue(current);
    *previous = current;
    return true;
  }
  return false;
}

bool detectButtonChange(bool* previous, int current, BLEBoolCharacteristic characteristic) {
  if (*previous == false && current == 4095) {
    characteristic.writeValue(true);
    *previous = true;
    return true;
  } else if (*previous == true && current != 4095) {
    characteristic.writeValue(false);
    *previous = false;
    return true;
  }
  return false;
}

void loop() {
  BLEDevice central = BLE.central();
  if (central) {

    Serial.print("Connected to central: ");
    Serial.println(central.address());

    while (central.connected()) {
      spr.fillSprite(TFT_BLACK);

      int ry = abs(analogRead(GPIO_NUM_15)-4096);
      int rx = analogRead(GPIO_NUM_14);
      int rw = analogRead(GPIO_NUM_13);
      int ly = analogRead(GPIO_NUM_12);
      int lx = analogRead(GPIO_NUM_11);
      int lw = analogRead(GPIO_NUM_10);
      spr.setTextSize(2);
      spr.setTextColor(TFT_YELLOW);
      String lStr = "l: (" + String(lx, DEC) + ", " + String(ly, DEC) + ")";
      String rStr = "r: (" + String(rx, DEC) + ", " + String(ry, DEC) + ")";
      spr.drawString(lStr, 0, 0);
      spr.drawString(rStr, 0, 16);
      if (lw == 4095) {
        spr.drawCircle(200, 8, 8, TFT_WHITE);
      }
      if (rw == 4095) {
        spr.drawCircle(200, 24, 8, TFT_WHITE);
      }
      detectChange(&lxLast, lx, lxChar);
      detectChange(&lyLast, ly, lyChar);
      detectButtonChange(&lwLast, lw, lwChar);
      detectChange(&rxLast, rx, rxChar);
      detectChange(&ryLast, ry, ryChar);
      detectButtonChange(&rwLast, rw, rwChar);
      spr.setTextSize(3);
      spr.setTextColor(TFT_GREEN);
      spr.drawString("BT", WIDTH - 48, HEIGHT - 24);
      spr.setTextSize(2);
      spr.setTextColor(TFT_WHITE);
      spr.drawString("batt:", 0, HEIGHT - 16);
      spr.setTextSize(3);
      spr.drawString(String(amoled.getBattVoltage() * 100 / 4095, DEC) + "%", 104, HEIGHT - 24);

      amoled.pushColors(0, 0, WIDTH, HEIGHT, (uint16_t*)spr.getPointer());
    }
    Serial.println("Disconnected from central");
  }

  spr.fillSprite(TFT_BLACK);

  spr.setTextSize(3);
        spr.setTextSize(2);
      spr.setTextColor(TFT_WHITE);
      spr.drawString("batt:", 0, HEIGHT - 16);
      spr.setTextSize(3);
      spr.drawString(String(amoled.getBattVoltage() * 100 / 4095, DEC) + "%", 104, HEIGHT - 24);

  spr.setTextColor(TFT_RED);
  spr.drawString("BT", WIDTH - 48, HEIGHT - 24);
  amoled.pushColors(0, 0, WIDTH, HEIGHT, (uint16_t*)spr.getPointer());

  if (digitalRead(GPIO_NUM_0) == LOW) {
    spr.fillSprite(TFT_BLACK);

    spr.setTextSize(5);

    spr.setTextColor(TFT_ORANGE);
    spr.drawString("Night night.", 0, 0);
    amoled.pushColors(0, 0, WIDTH, HEIGHT, (uint16_t*)spr.getPointer());
    delay(1000);
    Serial.println("night night");

    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, LOW);

    esp_deep_sleep_start();
  }

  delay(100);
}
