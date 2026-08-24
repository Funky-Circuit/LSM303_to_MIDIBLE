#include <NimBLEDevice.h>

#include <Adafruit_LSM303_Accel.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

#include <Adafruit_NeoPixel.h>





// -- -- -- -- --





//BLE
#define MIDI_SERVICE_UUID        "03B80E5A-EDE8-4B33-A751-6CE34EC4C700"
#define MIDI_CHARACTERISTIC_UUID "7772E5DB-3868-4112-A1A9-F2669D106BF3"
//
NimBLECharacteristic* pCharacteristic;
//
class ServerCallbacks : public NimBLEServerCallbacks 
{
  void onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc)
  {
    pServer->updateConnParams(desc->conn_handle, 6, 6, 0, 60);
  }

  void onDisconnect(NimBLEServer* pServer) 
  {
    NimBLEDevice::startAdvertising();
  }
};
//
uint8_t BLE_data_buffer[80];
uint8_t BLE_buffer_index = 0;
uint8_t last_millis_read;


//LSM303
Adafruit_LSM303_Accel_Unified accel = Adafruit_LSM303_Accel_Unified(54321);


//NeoPixel
Adafruit_NeoPixel pixels(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);


//General variables
struct axis_reading
{
  float x;
  float y;
  float z;
};

axis_reading current_axis_reading;
axis_reading last_axis_reading;

enum active_axis
{
  x,
  y,
  z,
  all
};

active_axis active_axis = x;

bool current_button_status;
bool last_button_status;





// -- -- -- -- --





void setup() 
{
  //Serial.begin(9600);
  //delay(1000);

  //BLE init
  NimBLEDevice::init("MIDI Accelerometer");
  NimBLEDevice::setMTU(185);
  //
  NimBLEServer* pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());
  //
  NimBLEService* pService = pServer->createService(MIDI_SERVICE_UUID);
  //
  pCharacteristic = pService->createCharacteristic(
    MIDI_CHARACTERISTIC_UUID,
    NIMBLE_PROPERTY::READ |
    NIMBLE_PROPERTY::WRITE_NR |
    NIMBLE_PROPERTY::NOTIFY
  );
  //
  pService->start();
  //
  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(MIDI_SERVICE_UUID);
  pAdvertising->start();


  //LSM303 init
  accel.begin();
  accel.setRange(LSM303_RANGE_4G);
  accel.setMode(LSM303_MODE_NORMAL);


  //NeoPixel init
  pixels.begin();
  pixels.clear();
  pixels.setPixelColor(0, pixels.Color(150, 0, 0));
  pixels.show();


  //GPIO init
  pinMode(0, INPUT_PULLUP);
}





// -- -- -- -- --





void BLE_MIDI_Control_Change(uint8_t channel, uint8_t function, uint8_t value)
{
  uint16_t timestamp = millis() & 0x1FFF;
  uint8_t header = 0x80 | ((timestamp >> 7) & 0x3F);
  uint8_t timeLow = 0x80 | (timestamp & 0x7F);
  if(BLE_buffer_index == 0)
  {
    BLE_data_buffer[BLE_buffer_index] = header; BLE_buffer_index ++;
  }
  BLE_data_buffer[BLE_buffer_index] = timeLow; BLE_buffer_index ++;
  BLE_data_buffer[BLE_buffer_index] = 0xB0 + channel; BLE_buffer_index ++;
  BLE_data_buffer[BLE_buffer_index] = function; BLE_buffer_index ++;
  BLE_data_buffer[BLE_buffer_index] = value; BLE_buffer_index ++;
}


void BLE_MIDI_send_buffer()
{
  if(BLE_buffer_index > 0)
  {
    if(millis() - last_millis_read > 5)
    {
      pCharacteristic->setValue(BLE_data_buffer, BLE_buffer_index);
      pCharacteristic->notify();
      BLE_buffer_index = 0;
      last_millis_read = millis();
    }
  }
}


void read_backup_axis_event(sensors_event_t event)
{
  last_axis_reading.x = current_axis_reading.x;
  last_axis_reading.y = current_axis_reading.y;
  last_axis_reading.z = current_axis_reading.z;
  current_axis_reading.x = event.acceleration.x;
  current_axis_reading.y = event.acceleration.y;
  current_axis_reading.z = event.acceleration.z;
}


void axis_selection()
{
  last_button_status = current_button_status;
  current_button_status = digitalRead(0);
  if(current_button_status != last_button_status && current_button_status == 0)
  {
    switch(active_axis)
    {
      case x:
        active_axis = y;
        pixels.clear();
        pixels.setPixelColor(0, pixels.Color(0, 150, 0));
        pixels.show();
        break;
      case y:
        active_axis = z;
        pixels.clear();
        pixels.setPixelColor(0, pixels.Color(0, 0, 150));
        pixels.show();
        break;
      case z:
        active_axis = all;
        pixels.clear();
        pixels.setPixelColor(0, pixels.Color(150, 150, 150));
        pixels.show();
        break;
      case all:
        active_axis = x;
        pixels.clear();
        pixels.setPixelColor(0, pixels.Color(150, 0, 0));
        pixels.show();
        break;
    }
  }
}





// -- -- -- -- --





void loop() 
{
  axis_selection();
  sensors_event_t event;
  accel.getEvent(&event);
  /*BLE_MIDI_Control_Change(0, 0, map(constrain(event.acceleration.x, -10, 10), -10, 10, 0, 127));
  BLE_MIDI_Control_Change(0, 1, map(constrain(event.acceleration.y, -10, 10), -10, 10, 0, 127));
  BLE_MIDI_Control_Change(0, 2, map(constrain(event.acceleration.z, -10, 10), -10, 10, 0, 127));*/
  read_backup_axis_event(event);
  if(current_axis_reading.x != last_axis_reading.x && (active_axis == x || active_axis == all))
  {
    BLE_MIDI_Control_Change(0, 1, map(constrain(current_axis_reading.x, -10, 10), -10, 10, 0, 127));
  }
  if(current_axis_reading.y != last_axis_reading.y && (active_axis == y || active_axis == all))
  {
    BLE_MIDI_Control_Change(0, 2, map(constrain(current_axis_reading.y, -10, 10), -10, 10, 0, 127));
  }
  if(current_axis_reading.z != last_axis_reading.z && (active_axis == z || active_axis == all))
  {
    BLE_MIDI_Control_Change(0, 3, map(constrain(current_axis_reading.z, -10, 10), -10, 10, 0, 127));
  }
  BLE_MIDI_send_buffer();
}
