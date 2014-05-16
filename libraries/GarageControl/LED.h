#ifndef __LED_h_
#define __LED_h_

#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

class LED
{
  public:
    LED(uint8_t pin = 13, bool on_is_high = true):
      m_pin(pin),
      m_on_is_high(on_is_high),
      m_value(0),
      m_program(NULL),
      m_frame(0)
    {
      pinMode(m_pin, OUTPUT);
      off();
    };
    
    ~LED()
    {
      pinMode(m_pin, OUTPUT);
      digitalWrite(m_pin, LOW);
    };

    LED(const LED & rhs)
    {
      *this = rhs;
    };
    
    LED & operator=(const LED & rhs)
    {
      if( this != &rhs )
      {
        this->m_pin = rhs.m_pin;
      }
        
      return *this;
    };

    uint8_t pin() const
    {
      return m_pin;
    };
    
    void on()
    {
      m_program = NULL;
      brightness(100);
    }
    
    void off()
    {
      m_program = NULL;
      brightness(0);
    }

    bool isOn() const
    {
      return brightness() > 50;
    }
    
    void brightness(uint8_t percent)
    {
      if( m_on_is_high )
      {
        m_value = percent * (255.0 / 100.0);
      }
      else
      {
        m_value = (100 - percent) * (255.0 / 100.0);        
      }
          
      analogWrite(m_pin, m_value);
    }

    uint8_t brightness() const
    {
      uint8_t percent;
      
      if( m_on_is_high )
      {
        percent = m_value * (100.0 / 255.0); 
      }
      else
      {
        percent = 100 - (m_value * (100.0 / 255.0));
      }
      
      return percent;
    }
    
    void flash()
    {
      if( isOn() )
      {        
        m_program = flash_inv_program;
        m_frame_count = 9;
      }
      else
      {
        m_program = flash_program;
        m_frame_count = 9;
      }

      m_frame = 0;
    }

    void fadeOn()
    {
      m_program = fade_on_program;
      m_frame_count = 5;
      m_frame = 0;
    }

    void fadeOff()
    {
      m_program = fade_off_program;
      m_frame_count = 5;
      m_frame = 0;
    }
  
    void display()
    {     
      if( (m_program != NULL) && (m_frame < m_frame_count) )
      {
        brightness(m_program[m_frame++]); 
      }
    }
    
  private:
    
    uint8_t m_pin;
    bool m_on_is_high;
    uint16_t m_value;
    const uint8_t * m_program;
    uint8_t m_frame;
    uint8_t m_frame_count;
    
    static uint8_t flash_program[9];
    static uint8_t flash_inv_program[9];
    static uint8_t fade_on_program[5];
    static uint8_t fade_off_program[5];
};

uint8_t LED::flash_program[9] = {
  0,25,50,75,100,75,50,25,0
};

uint8_t LED::flash_inv_program[9] = {
  100,75,50,25,0,25,50,75,100
};

uint8_t LED::fade_on_program[5] = {
  0,25,50,75,100
};

uint8_t LED::fade_off_program[5] = {
  100,75,50,25,0
};

#endif // __LED_h_
