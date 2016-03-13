#include <Wire.h>
#include <Adafruit_LEDBackpack.h>
#include <Adafruit_GFX.h>
#include <CmdMessenger.h>
#include <GarageControl.h>
#include <Button.h>
#include <LED.h>
#include <Timer.h>

#include<stdlib.h>

enum MODE
{
  ShowTemp,
  ShowHumid,
  ShowDoor,
  LAST
};

Button openButton(2);
Button closeButton(3);

LED openLed(5);
LED closeLed(6);

Timer LedTimer;

Button modeButton(7);
MODE mode = ShowTemp;

CmdMessenger cmd(Serial);

#define MATRIX_COUNT 2

Adafruit_8x8matrix matrix[] = {
  Adafruit_8x8matrix(),
  Adafruit_8x8matrix()
};

Timer d_timer;
String d_text;
int8_t d_length = 0;
int8_t d_cursor = 0;
bool d_sleft = true;

void OnTemperature()
{
  float value = 0.0;
  char buff[32] = {0};
  char suffix[2] = {0};

  if( mode == ShowTemp )
  {
    value = cmd.readFloatArg();
    suffix[0] = char(0xf7);
  }
  else
  if( mode == ShowHumid )
  {
    value = cmd.readFloatArg();
    value = cmd.readFloatArg();
    suffix[0] = '%';
  }

  if( value == int(value) )
  {
    itoa(int(value), buff, 10);
  }
  else
  {
    dtostrf(value, 4, 1, buff);
  }

  set_text(strcat(buff, suffix));
  cmd.sendCmd(GC_Acknowledge, "temp");
}

void OnDoorStatus()
{
  DS door = (DS)cmd.readIntArg();
  String s_door = toString(door);
  
  if( mode == ShowDoor )
  {
    switch( door )
    {
      case DS_Open:
        set_text("Open");
      break;

      case DS_Closed:
        set_text("Closed");
      break;
      
      default:
        set_text("");
    }
  }

  if( door == DS_Open )
  {
    openLed.on();
  }
  else
  {
    openLed.brightness(10);
  }

  if( door == DS_Closed )
  {
    closeLed.on();
  }
  else
  { 
    closeLed.brightness(10);
  }

  cmd.sendCmd(GC_Acknowledge, "door");
}

void set_text(const String & text)
{
  if( text != d_text )
  {
    d_text = text;
    d_length = (d_text.length() * 6) - 16;
    d_cursor = 0;
    d_timer.start(1);
//    cmd.sendCmd(GC_Acknowledge, d_length);
  }
}

void show_next_frame()
{
  for( uint8_t m = 0; m < MATRIX_COUNT; m++ )
  {
    matrix[m].clear();
   
    if( d_text.length() > 0 )
    {
      matrix[m].setTextSize(1);
      matrix[m].setTextWrap(false);  // we dont want text to wrap so it scrolls nicely
      matrix[m].setTextColor(LED_ON);
      matrix[m].setCursor(d_cursor - (8 * m), 0);
      matrix[m].print(d_text);
    }
    
    matrix[m].writeDisplay();
  }
  
  if( d_length > 0 )
  {
    if( d_cursor <= -d_length )
    {  
      d_sleft = false;
      d_timer.start(3000);
    }
    else
    if( d_cursor == 0 )
    {
      d_sleft = true;
      d_timer.start(3000);
    }
    else
    {
      d_timer.start(100);
    }
    
    d_cursor = (d_sleft ? d_cursor - 1 : d_cursor + 1);
  }
}

// Setup function
void setup() 
{  
  Serial.begin(9600); 
  Wire.begin();
  
  // Adds newline to every command
  cmd.printLfCr();   
  
  cmd.attach(GC_DoorStatus,  &OnDoorStatus);
  cmd.attach(GC_Temperature, &OnTemperature);
 
  cmd.sendCmd(GC_Acknowledge, F("Garage Control Master Started!"));

  for( uint8_t i = 0; i < MATRIX_COUNT; i++ ) 
  {
    matrix[i].begin(0x70 + i);
    matrix[i].clear();
    matrix[i].writeDisplay();
  }
    
  cmd.sendCmd(GC_GetTemperature, "temp?");
  cmd.sendCmd(GC_GetDoorStatus, "door?");
  
  // test indicators
  openLed.on();
  closeLed.on();  
  set_text("***");
  show_next_frame();
  
  delay(1000);

  closeLed.off();
  openLed.off();  
  set_text("");
  show_next_frame();

  d_timer.start(1);
  LedTimer.interval(30);
}

// Loop function
void loop() 
{
  // process serial commands
  cmd.feedinSerialData();
  
  if( openButton.closed() )
  {
    cmd.sendCmd(GC_OpenDoor, "Open the door!");
    openLed.flash();
  }
  
  if( closeButton.closed() )
  {
    cmd.sendCmd(GC_CloseDoor, "Close the door!");
    closeLed.flash();
  }

  if( modeButton.closed() )
  {
    cmd.sendCmd(GC_GetTemperature, "temp?");
    cmd.sendCmd(GC_GetDoorStatus, "door?");
    
    mode = (MODE)(mode + 1);
    
    if( mode == LAST )
    {
      mode = (MODE)0;
    }
  }

  if( d_timer.isTime() )
  {
    show_next_frame();
  }
  
  if( LedTimer.isTime() )
  {
    closeLed.display();
    openLed.display();
  }
}

