#include "button.h"

// debounce time in milliseconds
// (increase if reading is noisy, decrease if presses are missed)
unsigned long DebouncedButton::s_debounceDelay = 0; // WIP (if >0 double presses are registered by mistake)

void DebouncedButton::update() {
  // read current raw value and check if noise or intentional press
  bool reading = digitalRead(m_pin);
  if (reading != m_prevReading) {
    m_lastDebounceTime = millis();
  }

  // check if reading was held long enough (debouncing) to set button state
  if ((millis() - m_lastDebounceTime) >= s_debounceDelay) {
    // intentional press/release - update button state history
    m_prevState = m_currState;
    m_currState = reading;
  }

  // save reading for debouncing during next update
  m_prevReading = reading;
}
bool DebouncedButton::isPressed() { return m_currState; }
bool DebouncedButton::wasJustPressed() { return (m_currState && !m_prevState); }
bool DebouncedButton::wasJustReleased() { return (!m_currState && m_prevState); }
