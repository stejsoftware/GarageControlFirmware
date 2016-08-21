#include "application.h"

#include "Adafruit-LED-Backpack/Adafruit_LEDBackpack.h"
#include "Adafruit-GFX/Adafruit_GFX.h"
#include "CmdMessenger/CmdMessenger.h"
#include "ArduinoLibrary/Led_Matrix.h"
#include "ArduinoLibrary/Button.h"
#include "ArduinoLibrary/Led.h"
#include "ArduinoLibrary/GarageControl.h"
#include "blynk/blynk.h"

// time defines
#define SECOND_MS 1000
#define MINUTE_MS 60000
#define HOUR_MS   3600000

// button functions
void onButtonPressed(Button & button);

// cloud functions
int funcDoor(String command);

// timer functions
void onFiveMinutes();
void onFiveSeconds();
void onThirtySeconds();

// other functions
void lcdPrint(int line, const String & text);
void debug(const String & text);
void openDoor();
void closeDoor();

// global things
Button openButton(4);
Button closeButton(5);
Led_Matrix screen(16, 8, 2, 1, 256);
Led openLed(2);
Led closeLed(3);
CmdMessenger cmd(Serial1);
Timer fiveMinuteTimer(MINUTE_MS * 5, onFiveMinutes);
Timer fiveSecondTimer(SECOND_MS * 5, onFiveSeconds);
Timer thirtySecondTimer(SECOND_MS * 30, onThirtySeconds);
WidgetLCD lcd(V2);
WidgetTerminal terminal(V3);

// global vars
int rssi = 0;
time_t last = 0;
bool send_temp = true;
double temperature = 0.0;
double humidity = 0.0;
DS door_status = DS_Unknown;

  // settings
int16_t brightness = 100;

SYSTEM_THREAD(ENABLED);

void updateDisplay()
{
  screen.clearText();

  screen
    << (int16_t) round(temperature)
    << char(0xf7) << F(" ") // a degree symbol
    //<< (int16_t) round(humidity) << F("% ")
    << toString(door_status)
  ;
}

void onTemperature()
{
  //debug("onTemperature()");

  temperature = cmd.readFloatArg();
  humidity = cmd.readFloatArg();

  Blynk.virtualWrite(V6, temperature);
  Blynk.virtualWrite(V7, humidity);
  
  cmd.sendCmd(GC_Acknowledge, F("temp"));
  updateDisplay();
}

void lcdPrint(int line, const String & text)
{
  String _text = text;
  
  while( _text.length() < 16 )
  {
    _text += F(" ");
  }
  
  lcd.print(0, line, _text);
}

void onDoorStatus()
{
  //debug("onDoorStatus()");

  door_status = (DS) cmd.readInt32Arg();

  lcdPrint(0, toString(door_status));
  
  openLed.off();
  closeLed.off();

  switch( door_status )
  {
    case DS_Open:
      openLed.on();
      break;
    
    case DS_Closed:
      closeLed.on();
      break;
  }
  
  cmd.sendCmd(GC_Acknowledge, F("door"));
  updateDisplay();
}

void onOpenPressed(Button & button)
{
  openDoor();
}

void onClosePressed(Button & button)
{
  closeDoor();
}

void onFiveMinutes()
{
  if( send_temp )
  {
    Particle.publish(F("Temp"), String::format(F("%f"), temperature), PRIVATE);
    send_temp = false;
  }
  else
  {
    Particle.publish(F("Humidity"), String::format(F("%f"), humidity), PRIVATE);
    send_temp = true;
  }
}

void onFiveSeconds()
{
  rssi = WiFi.RSSI();
  Blynk.virtualWrite(V8, rssi);
}

void onThirtySeconds()
{
  cmd.sendCmd(GC_GetDoorStatus, F("door?"));
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
    openDoor();
    status = 3;
  }
  else
  if( strcmp(command, "Close") == 0 )
  {
    closeDoor();
    status = 4;
  }

  return status;
}

void openDoor()
{
  debug("Open Door!");
  openLed.flash();
  cmd.sendCmd(GC_OpenDoor, F("Open the door!"));
  Particle.publish("Button", "Open");
}

void closeDoor()
{
  debug("Close Door!");
  closeLed.flash();
  cmd.sendCmd(GC_CloseDoor, F("Close the door!"));  
  Particle.publish("Button", "Close");
}

BLYNK_WRITE(V0) {
  if (param.asInt() == 1) { // On button down...
    openDoor();
  }
}

BLYNK_WRITE(V1) {
  if (param.asInt() == 1) { // On button down...
    closeDoor();
  }
}

void debug(const String & text)
{
  terminal.print(Time.format(Time.now(), "%H:%M:%S "));
  terminal.println(text);
  terminal.flush();
}

// Setup function
void setup()
{
  // connection to the salve computer in the garage
  Serial1.begin(9600);

  // register Particle variables
  Particle.variable("rssi", rssi);
  Particle.variable("temperature", temperature);
  Particle.variable("humidity", humidity);

  // register Particle functions
  Particle.function("Door", funcDoor);

  // connect to the blynk server
  Blynk.begin("7cb12eb10dee49f98c61d3e7b34f7433");

  while (Blynk.connect() == false) {
    // Wait until connected
  }

  debug("STEJ Garage Control");
  debug(String::format("Built: %s %s",__DATE__, __TIME__));
  
  closeLed.setMax(brightness);
  openLed.setMax(brightness);

  openButton.attach(bePressed, onOpenPressed);
  closeButton.attach(bePressed, onClosePressed);

  screen.begin(0x73, true);
  screen.setBrightness((brightness / 100.0) * 15);
  screen.clearText();
  
  screen << "Now is the time for a very long message to see if it works!";
  
  lcd.clear();

  // Adds newline to every command
  cmd.printLfCr();

  cmd.attach(GC_DoorStatus, &onDoorStatus);
  cmd.attach(GC_Temperature, &onTemperature);

  cmd.sendCmd(GC_Acknowledge, F("Garage Control Master Started!"));

  onFiveSeconds(); fiveSecondTimer.start();
  fiveMinuteTimer.start();
  onThirtySeconds(); thirtySecondTimer.start();
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
  screen.run(mpScrollLeft, 100);

  // update leds
  closeLed.display();
  openLed.display();

  // update blynk
  Blynk.run();
}
