#include "application.h"

#include "Adafruit-LED-Backpack/Adafruit_LEDBackpack.h"
#include "Adafruit-GFX/Adafruit_GFX.h"
#include "CmdMessenger/CmdMessenger.h"
#include "ArduinoLibrary/Led_Matrix.h"
#include "ArduinoLibrary/Button.h"
#include "ArduinoLibrary/Led.h"
#include "ArduinoLibrary/Timer.h"
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
void onTenSecondTimer();

// event functions
void reportHandler(const char * event, const char * data);

// global things
Button openButton(4);
Button closeButton(5);
Led_Matrix screen(16, 8, 2, 1);
Led openLed(2);
Led closeLed(3);
CmdMessenger cmd(Serial1);
Config config;
Timer tenSecondTimer(SECOND_MS * 10, onTenSecondTimer); // trigger ever 10 seconds

// global vars
double upTime = 0.0;
int rssi = 0;
bool signalReport = false;
float temperature = 0.0;
float humidity = 0.0;
DS door_status = DS_Unknown;
MODE mode = RUN;
MODE submode = BRIGHT;
Alarm * watchdog = NULL;

  // settings
int16_t brightness = 100;
bool celsius = false;
bool fast = true;

SYSTEM_THREAD(ENABLED);
//SYSTEM_MODE(SEMI_AUTOMATIC);

void update_display()
{
  screen.clearText();
  screen.setBrightness((brightness / 100.0) * 15);
  
  closeLed.setMax(brightness);
  openLed.setMax(brightness);
  
  switch( mode )
  {
    case RUN:
      {
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
          openLed.off();
          closeLed.off();
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
        screen << (int16_t) round(temperature)
            << char(0xf7) << F(" ") // a degree symbol
            << (int16_t) round(humidity) << F("% ")
            << toString(door_status);
      }
    }
    break;

    case SETUP:
    {
      screen.resetFrame();
      openLed.on();
      closeLed.on();
      
      if( watchdog != NULL )
      {
        watchdog->reset();
      }
      
      switch( submode )
      {
        case BRIGHT:
          screen << brightness << "%" << F("<<Brightness");
        break;

        case TEMP:
          screen << (celsius ? "C" : "F") << F("<<Temp Scale");
        break;

        case SPEED:
          screen << (fast ? "Fast" : "Slow") << F("<<Scroll Speed");
        break;
        
        default:
          ; // do nothing
      }
    }
    break;
    
    default:
      ; // do nothing
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

void change_value(int8_t inc)
{
  switch( submode )
  {
    case BRIGHT:
      brightness += (inc * 10);

      if( brightness < 0 )
      {
        brightness = 0;
      }    
      
      if( brightness > 100 )
      {
        brightness = 100;
      }    
    break;
    
    case TEMP:
      celsius = !celsius;
    break;
    
    case SPEED:
      fast = !fast;
    break;
    
    default:
      ; // do nothing
  }
  
  update_display();
}

void open_pressed(Button & button)
{
  if (mode == RUN)
  {
    cmd.sendCmd(GC_OpenDoor, F("Open the door!"));
    Particle.publish("Button", "Open", PRIVATE);
  }
  else
  {
    change_value(-1);
  }
}

void close_pressed(Button & button)
{
  if (mode == RUN)
  {
    cmd.sendCmd(GC_CloseDoor, F("Close the door!"));
    Particle.publish("Button", "Close", PRIVATE);
  }
  else
  {
    change_value(+1);
  }
}

void mode_click(Button & button)
{
  if (mode == RUN)
  {
    get_status();
  }
  else
  {
    switch( submode )
    {
      case BRIGHT:
        submode = TEMP;
      break;

      case TEMP:
        submode = SPEED;
      break;

      case SPEED:
        submode = BRIGHT;
      break;
      
      default:
        ; // do nothing
   }

    update_display();
  }
}

void mode_held(Button & button)
{
  switch( mode )
  {
    case RUN:
      mode = SETUP;
      
      if( watchdog == NULL )
      {
        watchdog = ArduinoTimer.delay(exit_setup, 30000);
      }      

      update_display();
    break;

    case SETUP:
      exit_setup();
    break;
    
    default:
      ; // do nothing
  }
}

void exit_setup()
{
  mode = RUN;
  
  if( watchdog != NULL )
  {
    watchdog->stop();
    watchdog = NULL;
  }  
  
  update_display();
}

void onTenSecondTimer()
{
  rssi = WiFi.RSSI();

  Particle.publish("Door", toString(door_status), PRIVATE);
  Particle.publish("Temp", String::format("%f", temperature), PRIVATE);
  Particle.publish("Humidity", String::format("%f", humidity), PRIVATE);
}

void reportHandler(const char * event, const char * data)
{
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

  get_status();

  Particle.variable("rssi", rssi);
  Particle.subscribe("report", &reportHandler);
  
  tenSecondTimer.start();
}

// Loop function
void loop()
{
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
