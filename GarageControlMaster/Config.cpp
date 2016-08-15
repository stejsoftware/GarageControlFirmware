/*
 * Config.cpp
 *
 *  Created on: Nov 17, 2015
 *      Author: Jonathan Meyer <jon@stejsoftware.com>
 */

#include "application.h"
#include "Json/ArduinoJson.h"
#include "Config.h"

#define CONFIG_VER 1

const String Config::EVENT_NAME = String("config/") + System.deviceID();
const String Config::SET_CONFIG = Config::EVENT_NAME + String("/set");
const String Config::GET_CONFIG = Config::EVENT_NAME + String("/get");

Config::Config()
{
}

Config::Config(const Config & rhs)
{
  *this = rhs;
}

Config & Config::operator=(const Config & rhs)
{
  if( this != &rhs )
  {
  }

  return *this;
}

Config::~Config()
{
}

//
// load / read from the EEPROM
//
void Config::load()
{
  uint32_t index = 0;

  uint32_t version = 0;
  EEPROM.get(index, version);
  index += sizeof(version);

  if( version == CONFIG_VER )
  {
//    EEPROM.get(index, m_sampleCount);
//    index += sizeof(m_sampleCount);
  }

  publish();
}

//
// save / write to the EEPROM
//
void Config::save()
{
  uint32_t index = 0;

  uint32_t version = CONFIG_VER;
  EEPROM.put(index, version);
  index += sizeof(version);

//  EEPROM.put(index, m_sampleCount);
//  index += sizeof(m_sampleCount);
}

void Config::subscribe()
{
  Particle.subscribe(EVENT_NAME.c_str(), &Config::handler, this);
  Particle.publish("INFO", EVENT_NAME.c_str(), PRIVATE);
}

template<class T>
T get(const JsonObject & obj, const JsonObject::key_type & key, const T & defalut_value)
{
  T value = defalut_value;

  if( obj.containsKey(key) )
  {
    value = obj[key];
  }

  return value;
}

void Config::handler(const char * event, const char * data)
{
  Serial.println(event);
  Serial.println(data);
  Serial.println();

  if( String(event).startsWith(SET_CONFIG) )
  {
    char buff[256] = { 0 };
    StaticJsonBuffer<256> jsonBuffer;
    strncpy(buff, data, sizeof(buff));
    //JsonObject & root = jsonBuffer.parseObject(buff, 2);

    //m_sampleCount = get(root, "count", 1);

    save();
    publish();
  }
  else
  if( String(event).startsWith(GET_CONFIG) )
  {
    publish();
  }
}

void Config::publish() const
{
  Particle.publish("config", String::format("nothing here yet"), PRIVATE);
}
