#ifndef __Timer_h_
#define __Timer_h_

#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

class Timer
{
  public:
    Timer():
      m_millis(0),
      m_interval(0),
      m_once(false)
    {
    };
      
    ~Timer()
    {
    };

    void start(uint32_t time)
    {
      m_once = true;
      m_millis = millis();
      m_interval = time;
    }
    
    void interval(uint32_t interval)
    {
      m_once = false;
      m_millis = millis();
      m_interval = interval;
    };
  
    bool isTime()
    {
      if( (m_interval > 0) && ((millis() - m_millis) > m_interval) )
      { 
        if( m_once )
        {
          m_interval = 0;
        }   
        else
        {
          m_millis = millis();
        }
              
        return true;
      }
      
      return false;
    };

  private:
    Timer(const Timer & rhs);
    Timer & operator=(const Timer & rhs);
    
    uint32_t m_millis;
    uint32_t m_interval;
    bool m_once;
};

#endif // __Timer_h_
