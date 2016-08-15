/*
 * Config.h
 *
 *  Created on: Nov 17, 2015
 *      Author: jon
 */

#ifndef Config_H_
#define Config_H_

class Config
{
public:
  Config();
  Config(const Config & rhs);
  Config & operator=(const Config & rhs);
  virtual ~Config();

  void load();
  void save();

  void publish() const;
  void subscribe();
  void handler(const char * event, const char * data);

private:
  static const String EVENT_NAME;
  static const String SET_CONFIG;
  static const String GET_CONFIG;

};

#endif /* Config_H_ */
