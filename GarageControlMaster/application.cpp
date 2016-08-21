#include "application.h"

#include "Adafruit-LED-Backpack/Adafruit_LEDBackpack.h"
#include "Adafruit-GFX/Adafruit_GFX.h"
#include "CmdMessenger/CmdMessenger.h"
#include "ArduinoLibrary/Led_Matrix.h"
#include "ArduinoLibrary/Button.h"
#include "ArduinoLibrary/Led.h"
#include "ArduinoLibrary/GarageControl.h"
#include "blynk/blynk.h"
#include "Config.h"

// time defines
#define SECOND_MS 1000
#define MINUTE_MS 60000
#define HOUR_MS   3600000

enum MODE
{
  RUN,
  SETUP,
  BRIGHT,
  TEMP,
  SPEED,
};

// cloud functions
int funcDoor(String command);

// timer functions
void onFiveMinutes();
void onFiveSeconds();

// global things
Button openButton(4);
Button closeButton(5);
Led_Matrix screen(16, 8, 2, 1, 256);
WidgetLCD lcd(V2);
WidgetTerminal terminal(V3);
Led openLed(2);
Led closeLed(3);
CmdMessenger cmd(Serial1);
Config config;
Timer fiveMinuteTimer(MINUTE_MS * 5, onFiveMinutes);
Timer fiveSecondTimer(SECOND_MS * 5, onFiveSeconds);

// global vars
int rssi = 0;
time_t last = 0;
bool send_temp = true;
double temperature = 0.0;
double humidity = 0.0;
DS door_status = DS_Unknown;

  // settings
int16_t brightness = 100;
bool celsius = false;
bool fast = true;

SYSTEM_THREAD(ENABLED);

void update_display()
{
  screen.clearText();
  lcd.clear();
  screen.setBrightness((brightness / 100.0) * 15);

  closeLed.setMax(brightness);
  openLed.setMax(brightness);

  bool moving_door = false;

  switch( door_status )
  {
    case DS_Open:
    case DS_Closed:
      openLed.off();
      closeLed.off();
    break;

    case DS_Opening:
      if( !openLed.isRunning() )
      {
        openLed.flash();
      }

      moving_door = true;
    break;

    case DS_Closing:
      if( !closeLed.isRunning() )
      {
        closeLed.flash();
      }

      moving_door = true;
    break;

    default:
      ; // do nothing
  }

  if (moving_door)
  {
    screen << F("** ") << toString(door_status);
  }
  else
  {
    screen
      << (int16_t) round(temperature)
      << char(0xf7) << F(" ") // a degree symbol
      //<< (int16_t) round(humidity) << F("% ")
      << toString(door_status)
      //<< F("Tabitha & Zibby! I don't know the true status of the door casue the sensors are broken! :P ")
      ;
      
    lcd.print(0, 0, toString(door_status));
    lcd.print(0, 1, String::format("%d%c %d%%",(int16_t) round(temperature), char(0xf7), (int16_t) round(humidity)));
  }
}

void OnTemperature()
{
  terminal.print(Time.format(Time.now(), "%H:%M:%S "));
  terminal.println("OnTemperature()");
  terminal.flush();
  
  temperature = cmd.readFloatArg();
  humidity = cmd.readFloatArg();

  update_display();

  cmd.sendCmd(GC_Acknowledge, F("temp"));
}

void OnDoorStatus()
{
  terminal.print(Time.format(Time.now(), "%H:%M:%S "));
  terminal.println("OnDoorStatus()");
  terminal.flush();

  door_status = (DS) cmd.readInt32Arg();

  update_display();

  cmd.sendCmd(GC_Acknowledge, F("door"));
}

void get_status()
{
  screen.clearText();
  screen << F("Querying the Garage");

  cmd.sendCmd(GC_GetTemperature, F("temp?"));
  cmd.sendCmd(GC_GetDoorStatus, F("door?"));
}

void open_pressed(Button & button)
{
  openLed.flash();
  cmd.sendCmd(GC_OpenDoor, F("Open the door!"));
  Particle.publish("Button", "Open", PRIVATE);
}

void close_pressed(Button & button)
{
  closeLed.flash();
  cmd.sendCmd(GC_CloseDoor, F("Close the door!"));
  Particle.publish("Button", "Close", PRIVATE);
}

void onFiveMinutes()
{
  if( send_temp )
  {
    Particle.publish("Temp", String::format("%f", temperature), PRIVATE);
    send_temp = false;
  }
  else
  {
    Particle.publish("Humidity", String::format("%f", humidity), PRIVATE);
    send_temp = true;
  }
}

void onFiveSeconds()
{
  rssi = WiFi.RSSI();
}

int funcDoor(String command)
{
  int status = -1;

  if( strcmp(command, "Status") == 0 )
  {
    status = door_status == DS_Open ? 1 : 0;
  }
  else
  if( strcmp(command, "Open") == 0 )
  {
    //cmd.sendCmd(GC_OpenDoor, F("Open the door!"));
    status = 3;
  }
  else
  if( strcmp(command, "Close") == 0 )
  {
    cmd.sendCmd(GC_CloseDoor, F("Close the door!"));
    status = 4;
  }

  return status;
}

BLYNK_WRITE(V1) {
  if (param.asInt() == 1) { // On button down...
    close_pressed(closeButton);
  }
}

// Setup function
void setup()
{
  Serial1.begin(9600);
  Serial1.print("ID: ");
  Serial1.println(System.deviceID());

  Blynk.begin("7cb12eb10dee49f98c61d3e7b34f7433");
  
  config.subscribe();
  config.load();

  openButton.attach(bePressed, open_pressed);
  closeButton.attach(bePressed, close_pressed);

  screen.begin(0x73, true);

  // Adds newline to every command
  cmd.printLfCr();

  cmd.attach(GC_DoorStatus, &OnDoorStatus);
  cmd.attach(GC_Temperature, &OnTemperature);

  cmd.sendCmd(GC_Acknowledge, F("Garage Control Master Started!"));

  Particle.variable("rssi", rssi);
  Particle.variable("temperature", temperature);
  Particle.variable("humidity", humidity);
  
  Particle.function("Door", funcDoor);

  fiveMinuteTimer.start();
  fiveSecondTimer.start();
}

// Loop function
void loop()
{
  if( (door_status == DS_Unknown) && (Time.now() > last + 30) )
  {
    get_status();
    last = Time.now();
  }

  // process serial commands
  cmd.feedinSerialData();

  // read buttons
  openButton.run();
  closeButton.run();

  // update led matrix
  screen.run(mpScrollLeft, (fast ? 100 : 200));

  // update leds
  closeLed.display();
  openLed.display();
  
  Blynk.run();
}
