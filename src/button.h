#ifndef __BUTTON_H__
#define __BUTTON_H__

#include <Arduino.h>

class DebouncedButton {
  /* Implements Button reading interface with adjustable debounce time.
   *
   * The .update() function must be called before reading the state of the button
   * in every cycle of the runtime loop() function.
   *
   * If spurious readings are observed, try increasing the s_debounceDelay property
   * which should filter out more noise on the raw button reading
   */
  public:
    DebouncedButton() {;}
    DebouncedButton(int pin) : m_pin(pin) {
      pinMode(pin, INPUT);
      m_prevReading = m_prevState = m_currState = digitalRead(m_pin);
      m_lastDebounceTime = millis();
    }

    // Update the state history of the button (with debouncing)
    // This should be called in every cycle of the runtime loop, before
    // reading the clean state of the button.
    void update();

    // True as long as button is held down
    bool isPressed();

    // True if button just changed from unpressed to pressed
    bool wasJustPressed();

    // True if button just changed from pressed to unpressed
    bool wasJustReleased();


  protected:
    static unsigned long s_debounceDelay; // debounce time in milliseconds

    unsigned long m_lastDebounceTime; // last time state changed in raw reading
    bool m_prevReading; // last instantaneous reading (before debouncing)

    // Debounced button states (length=2 history necessary for discerning justPressed/justReleased)
    bool m_currState;   // current debounced state
    bool m_prevState;   // last debounced state
    int m_pin;          // pin to read
};

#endif // __BUTTON_H__
