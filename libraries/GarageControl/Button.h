#ifndef __Button_h_
#define __Button_h_

#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

class Button
{
  public:
    Button(uint8_t pin, uint8_t closed_value):
      m_pin(pin),
      m_closed_is_high(closed_value == HIGH),
      m_time(0),
      m_click_start(0),
      m_debounce_delay(50), // the amount of time required to identify a latch
      m_click_delay(500),   // the max amount of time required to identify a click 
      m_last_read(false),
      m_current_state(false),
      m_click(false),
      m_closed(false),
      m_opened(false)
    {
      pinMode(m_pin, INPUT);
    };
      
    ~Button()
    {
      pinMode(m_pin, OUTPUT);
      digitalWrite(m_pin, LOW);
    };

    bool click()
    {
      read();
      
      if( m_click )
      {
        m_click = false;
        return true;
      }
      
      return false;
    };
    
    bool closed()
    {
      read();
      
      if( m_closed )
      {
        m_closed = false;
        return true;
      }
      
      return false;
    }

    bool opened()
    {
      read();
      
      if( m_opened )
      {
        m_opened = false;
        return true;
      }
      
      return false;
    }
    
    uint8_t read()
    {
      // read the button
      bool current_read = m_closed_is_high ? (digitalRead(m_pin) == HIGH) : (digitalRead(m_pin) == LOW);

      // if the button state chagned
      if( current_read != m_last_read )
      {
        // reset the timer
        m_time = millis();
        
        // save the current read as the last read
        m_last_read = current_read;
      }

      // if the timer has been running long enough
      // set the acutal state to the current state
      if( (millis() - m_time) > m_debounce_delay )
      {
        // if latch up
        if( m_last_read && !m_current_state )
        {
          // trigger closed event
          m_closed = true;
            
          // start the click timer if it is not running
          if( m_click_start == 0 )
          {
            m_click_start = millis();
          }
        }
        else
        // if latch down
        if( !m_last_read && m_current_state )
        {
          // tirgger open event
          m_opened = true;
          
          // check to see if the button was pressed long enough 
          // to identify a click
          if( m_click_start > 0 )
          {            
            if( (millis() - m_click_start) <= m_click_delay )
            {
              m_click = true;
            }
          
            // debug: print actual delay
            //Serial.println(delay);
          }
          
          // stop the click timer
          m_click_start = 0;
        }
        
        // save the current state
        m_current_state = m_last_read;
        
      } // endif delay enough
        
      // return the actual state
      return m_closed_is_high ? (current_read ? HIGH : LOW) : (current_read ? LOW : HIGH);
    };

  private:
    Button(const Button & rhs);
    Button & operator=(const Button & rhs);
    
    uint32_t m_time;
    uint32_t m_click_start;
    uint8_t m_pin;
    uint8_t m_debounce_delay;
    uint8_t m_click_delay;
    bool m_last_read;
    bool m_current_state;
    bool m_closed_is_high;    

    // events
    bool m_click;
    bool m_closed;
    bool m_opened;

};

#endif // __Button_h_
