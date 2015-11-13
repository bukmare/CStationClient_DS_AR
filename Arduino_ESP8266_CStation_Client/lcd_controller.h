#ifndef LCD_CONTROLLER_H
#define LCD_CONTROLLER_H

#define LCD_I2C_ADDR 0x3F
//#define LCD_I2C_ADDR 0x27

#define LCD_AUTO_TURNOFF_MSTIME 100000
#define LCD_AUTO_TURNPAGE_MSTIME 7000
#define LCD_AUTO_UPDTIME_MSTIME 30000

#define LCD_PAGES_COUNT 4
#define LCD_PAGE_SYSTEM 0
#define LCD_PAGE_OUTER 1
#define LCD_PAGE_SENSORS 2
#define LCD_PAGE_TIME 3

class LCDController 
{
  private:
    LiquidCrystal_I2C *lcd;
    
    bool fixed_page;
    bool lcd_auto_state;
    bool lcd_ison;
    unsigned long int last_auto_state;
    unsigned long int last_pager_state;
    unsigned long int last_time_state;
    char line1_dyn[LCD_PAGES_COUNT][17];
    char line2_dyn[LCD_PAGES_COUNT][17];
    bool text_changed;
    byte page_num;
    byte page_to;

  public:
    static LCDController *_self_controller;

    static LCDController* Instance() {
      if(!_self_controller)
      {
          _self_controller = new LCDController();
      }
      return _self_controller;
    }
    static bool DeleteInstance() {
      if(_self_controller)
      {
          delete _self_controller;
          _self_controller = NULL;
          return true;
      }
      return false;
    }

    LCDController() 
    {
      last_auto_state = 0;
      last_pager_state = 0;
      last_time_state = 0;
      lcd = new LiquidCrystal_I2C(LCD_I2C_ADDR, 16, 2);
      initLCD();
    }

    void initLCD()
    {
      for(byte i=0; i<LCD_PAGES_COUNT; i++) {
        line1_dyn[i][0] = 0;
        line2_dyn[i][0] = 0;
      }
      text_changed = true;
      lcd_ison = true;
      lcd_auto_state = true;
      fixed_page = false;
      page_num = 0;
      page_to = 0;
      lcd->init();
      lcd->backlight();
      lcd->home(); 
      lcd->clear(); 
      last_auto_state = last_pager_state = last_time_state = millis();
      showCurrentPage();
    }

    void timerProcess()
    {
      unsigned long int cmilli = millis();
      if (cmilli-last_time_state > LCD_AUTO_UPDTIME_MSTIME) {
        last_time_state = cmilli;
        if (timeStatus()!=timeNotSet) {
          String ts = String(hour())+":";
          if (minute()<10) ts+="0";
          ts+=String(minute());
          if (ts.length()<5) ts="      "+ts; else ts="     "+ts;
          String ts2 = String(day())+".";
          if (month()<10) ts2+="0";
          ts2+=String(month())+"."+String(year());
          if (ts2.length()>9) ts2="   "+ts2; else ts2="    "+ts2;
          setLCDLines(ts.c_str(), ts2.c_str(), LCD_PAGE_TIME);
        }
      }
      if (cmilli-last_pager_state > LCD_AUTO_TURNPAGE_MSTIME) {
        last_pager_state = cmilli;
        nextPage();
      }
    }

    void redrawTimePage() {
      last_time_state = 0;
      timerProcess();
      if (page_num == LCD_PAGE_TIME) {
        showCurrentPage();
      }
    }

    void setLCDState(bool ison)
    {
      lcd_ison = ison;
      lcd_auto_state = false;
      if (lcd_ison) {
        lcd->backlight();
      } else {
        lcd->noBacklight();
      }
    }
    
    void setLCDAutoState()
    {
      lcd_auto_state = true;
      updateLCDAutoState();
    }
    
    void updateLCDAutoState()
    {
      if (lcd_auto_state) {
        last_auto_state = millis();
        if (!lcd_ison) {
          lcd->backlight();
          lcd_ison = true;
        }
      }
    }

    void fixPage(byte page_fix) 
    {
      page_num = page_fix;
      fixed_page = true;
      showCurrentPage();
    }

    void unfixPage() 
    {
      fixed_page = false;
      last_pager_state = millis();
      nextPage();
    }

    void showCurrentPage()
    {
      if (text_changed) {
        lcd->home(); 
        lcd->clear(); 
        lcd->print(line1_dyn[page_num]);
        lcd->setCursor(0, 1);
        lcd->print(line2_dyn[page_num]);
        text_changed = false;
      }
      checkLCDAutoState();
    }

    void clearLCDText(byte page)
    {
      line1_dyn[page][0] = 0;
      line2_dyn[page][0] = 0;
      if (page == page_num) {
        text_changed = true;
        showCurrentPage();
      }
    }
    
    void setLCDText(const char* text, byte page_to) 
    {
      strncpy(line1_dyn[page_to], text, 17);
      if (strlen(text)>16) {
        strncpy(line2_dyn[page_to], text+16, 17);
      } else {
        line2_dyn[page_to][0] = 0;
      }
      text_changed = true;
      showCurrentPage();
      checkLCDAutoState();
    }

    void setLCDText(const char* text) 
    {
      setLCDText(text, LCD_PAGE_SYSTEM);
    }
    
    void setLCDLines(const char* line1, const char* line2, byte page_to) 
    {
      strncpy(line1_dyn[page_to], line1, 17);
      strncpy(line2_dyn[page_to], line2, 17);
      text_changed = true;
      showCurrentPage();
    }

    void setLCDLines(const char* line1, const char* line2) 
    {
      setLCDLines(line1, line2, LCD_PAGE_SYSTEM);
    }

    void setPageTo(byte new_page_to)
    {
      page_to = new_page_to;
    }

    void nextPage()
    {
      if (!fixed_page) {
        byte next_page = nextNotEmptyPage();
        if (next_page!=page_num) {
          text_changed = true;
          page_num = next_page;
        }
      }
      showCurrentPage();
    }

    byte getCurrentPage()
    {
      return page_num;
    }

    bool pageIsEmpty(byte page) 
    {
      return !line1_dyn[page][0] && !line2_dyn[page][0];
    }

  protected:

    byte nextNotEmptyPage()
    {
      byte tmp_page = page_num;
      byte i=0;
      do {
        tmp_page = (tmp_page+1) % LCD_PAGES_COUNT;
        i++;
      } while (i<LCD_PAGES_COUNT && pageIsEmpty(tmp_page));
      return tmp_page;
    }

    void checkLCDAutoState()
    {
      if (lcd_auto_state) {
        if (lcd_ison && millis()-last_auto_state > LCD_AUTO_TURNOFF_MSTIME) {
          lcd->noBacklight();
          lcd_ison = false;
        }
      }
    }
};

LCDController *LCDController::_self_controller = NULL;

#endif
