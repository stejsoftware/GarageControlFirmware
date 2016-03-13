#include "Wire.h"
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"
#include "CmdMessenger.h"
#include "GarageControl.h"
#include "Led_Matrix.h"
#include "Button.h"
#include "Led.h"
#include "Timer.h"

enum MODE
{
  RUN,
  SETUP,
  BRIGHT,
  TEMP,
  SPEED,
};

Button openButton(2);
Button closeButton(3);
Button modeButton(7);

Led_Matrix screen(16, 8, 2, 1);

Led openLed(5);
Led closeLed(6);

CmdMessenger cmd(Serial);

float temperature = 0.0;
float humidity = 0.0;
DS door_status = DS_Unknown;
MODE mode = RUN;
MODE submode = BRIGHT;
Alarm * watchdog = NULL;

// settings
int8_t brightness = 50;
bool celsius = false;
bool fast = true;

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
  screen << F("**");
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
        watchdog = Timer.delay(exit_setup, 30000);
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

// Setup function
void setup()
{
  Serial.begin(9600);

  openButton.attach(bePressed, open_pressed);
  closeButton.attach(bePressed, close_pressed);
  modeButton.attach(beClick, mode_click);
  modeButton.attach(beHeld, mode_held);

  screen.begin();

  // Adds newline to every command
  cmd.printLfCr();

  cmd.attach(GC_DoorStatus, &OnDoorStatus);
  cmd.attach(GC_Temperature, &OnTemperature);

  cmd.sendCmd(GC_Acknowledge, F("Garage Control Master Started!"));

  get_status();
}

// Loop function
void loop()
{
  // process serial commands
  cmd.feedinSerialData();

  // read buttons
  openButton.run();
  closeButton.run();
  modeButton.run();

  // update led matrix
  screen.run(mpScrollLeft, (fast ? 100 : 200));

  // update leds
  closeLed.display();
  openLed.display();
  
  // timer
  Timer.run();
}

