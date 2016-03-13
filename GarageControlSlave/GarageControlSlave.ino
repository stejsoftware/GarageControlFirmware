#include <DHT.h>
#include <CmdMessenger.h>
#include <GarageControl.h>
#include <Timer.h>
#include <LED.h>
#include <Button.h>
#include <Relay.h>

// door status
Button door_open(11, false);
Button door_closed(10, false);
Timer door_timer;
DS door_status = DS_Unknown;

// relay variables
Relay relay(9);

// temp variables
DHT sensor(2, DHT22);
Timer temp_timer;
float temperature = 0.0;
float humidity = 0.0;

// button vars
Button button(12, false);
LED button_led(5);

CmdMessenger cmd(Serial);
LED green(13);
Timer led_timer;

void push_button()
{
  relay.flash();
}

void OnPushButton()
{
  cmd.sendCmd(GC_Acknowledge, "OnPushButton");  
  push_button();
}

void OnCloseDoor()
{
  if( door_status == DS_Open )
  {
    push_button();
    cmd.sendCmd(GC_Acknowledge, "OnCloseDoor");
  }
  else
  {  
    cmd.sendCmd(GC_Error, "OnCloseDoor: Door is not open.");  
  }
}

void OnOpenDoor()
{
  if( door_status == DS_Closed )
  {
    push_button();
    cmd.sendCmd(GC_Acknowledge, "OnOpenDoor");
  }
  else
  {
    cmd.sendCmd(GC_Error, "OnOpenDoor: Door is not closed.");  
  }
}

void OnGetTemperature()
{
  cmd.sendCmdStart(GC_Temperature);
  cmd.sendCmdArg(sensor.convertCtoF(temperature));
  cmd.sendCmdArg(humidity);
  cmd.sendCmdEnd();
  green.flash();
}

void read_sensor()
{
  float temp = sensor.readTemperature();
  float humd = sensor.readHumidity();

  // if we didn't get numbers then there is an error
  if( !isnan(temp) && !isnan(humd) )
  {
    // if either the temp or humid changes
    if( (temp != temperature) || (humd != humidity) )
    {
      // store the new values
      temperature = temp;
      humidity = humd;

      // report the values
      OnGetTemperature();
    }
  }
  else
  {
    cmd.sendCmd(GC_Error, "Couldn't read the Sensor.");
  }
}

void get_door_status()
{
  DS was = door_status;
  
  if( door_open.closed() )
  {
      door_status = DS_Open;
      button_led.on();
  }
  else
  if( door_closed.closed() )
  {
      door_status = DS_Closed;
      button_led.off();
  }
  else
  {
/*    if( door_status == DS_Open )
    {
      door_status = DS_Closing;
    }
    else
    if( door_status == DS_Closed )
    {
      door_status = DS_Opening;      
    }
*/
  }
  
  if( door_status != was )
  {
    door_timer.start(100);
  }
}

void OnGetDoorStatus()
{
  cmd.sendCmd(GC_DoorStatus, door_status);
  green.flash();
}

// Setup function
void setup() 
{  
  // Listen on serial connection for messages from the master
  Serial.begin(9600); 
  
  // start sensor
  sensor.begin();
  
  // Adds newline to every command
  cmd.printLfCr();   
  
//  cmd.attach(GC_PushButton,     &OnPushButton);
  cmd.attach(GC_CloseDoor,      &OnCloseDoor);
  cmd.attach(GC_OpenDoor,       &OnOpenDoor);
  cmd.attach(GC_GetTemperature, &OnGetTemperature);
  cmd.attach(GC_GetDoorStatus,  &OnGetDoorStatus);    
//  cmd.attach(GC_SetDoorInterval, &OnSetDoorInterval);
    
  temp_timer.interval(10000); // 10 sec
  led_timer.interval(30);
  
  cmd.sendCmd(GC_Acknowledge, "Garage Control Slave Started!");
}

// Loop function
void loop() 
{
  // process serial commands
  cmd.feedinSerialData();

  if( temp_timer.isTime() )
  {
    // read the sensor and report if changes
    read_sensor();
  }

  // check the button
  if( button.closed() )
  {
    button_led.flash();
    OnPushButton();
  }
  
  // update the status of the door
  get_door_status();

  if( door_timer.isTime() )
  {
    // report the door status
    OnGetDoorStatus();
  }
  
  if( led_timer.isTime() )
  {
    button_led.display();
    green.display();
    relay.run();
  }
}

