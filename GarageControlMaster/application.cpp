#include "application.h"

#include "Adafruit-LED-Backpack/Adafruit_LEDBackpack.h"
#include "Adafruit-GFX/Adafruit_GFX.h"
#include "CmdMessenger/CmdMessenger.h"
#include "ArduinoLibrary/Led_Matrix.h"
#include "ArduinoLibrary/Button.h"
#include "ArduinoLibrary/Led.h"
#include "ArduinoLibrary/GarageControl.h"
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

// other functions
void exit_setup();

// cloud functions
int funcRestart(String command);
int funcReport(String command);

// timer functions
void onFiveMinutes();
void onFiveSeconds();

// global things
Button openButton(4);
Button closeButton(5);
Led_Matrix screen(16, 8, 2, 1, 256);
Led openLed(2);
Led closeLed(3);
CmdMessenger cmd(Serial1);
Config config;
Timer fiveMinuteTimer(MINUTE_MS * 5, onFiveMinutes);
Timer fiveSecondTimer(SECOND_MS * 5, onFiveSeconds);

// global vars
int count = 0;
double upTime = 0.0;
int rssi = 0;
time_t last = 0;
bool send_temp = true;
float temperature = 0.0;
float humidity = 0.0;
DS door_status = DS_Unknown;

  // settings
int16_t brightness = 100;
bool celsius = false;
bool fast = true;

SYSTEM_THREAD(ENABLED);
//SYSTEM_MODE(SEMI_AUTOMATIC);

void update_display()
{
  count++;
  screen.clearText();
  screen.setBrightness((brightness / 100.0) * 15);
  
  closeLed.setMax(brightness);
  openLed.setMax(brightness);
  
  bool moving_door = false;

  switch( door_status )
  {
    case DS_Open:
      closeLed.off();
      openLed.on();
    break;

    case DS_Closed:
      openLed.off();
      closeLed.on();
    break;

    case DS_Opening:
    case DS_Closing:
      if( !openLed.isRunning() )
      {
        openLed.flash();
      }
      
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
  }
}

void OnTemperature()
{
  temperature = cmd.readFloatArg();
  humidity = cmd.readFloatArg();
 
  update_display();
  
  cmd.sendCmd(GC_Acknowledge, F("temp"));
}

void OnDoorStatus()
{
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
  cmd.sendCmd(GC_OpenDoor, F("Open the door!"));
  Particle.publish("Button", "Open", PRIVATE);
}

void close_pressed(Button & button)
{
  cmd.sendCmd(GC_CloseDoor, F("Close the door!"));
  Particle.publish("Button", "Close", PRIVATE);
}

void onFiveMinutes()
{
  Particle.publish("Door", toString(door_status), PRIVATE);
  
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

// Setup function
void setup()
{
  Serial1.begin(9600);
  Serial1.print("ID: ");
  Serial1.println(System.deviceID());

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
  Particle.variable("door", door_status);
  Particle.variable("count", count);
  
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
}
