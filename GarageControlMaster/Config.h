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

};

#endif /* Config_H_ */
