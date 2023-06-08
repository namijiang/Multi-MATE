/* Author: Ryan Neph
   Last Updated: 13 Jan 2020
*/

#include <Arduino.h>
#include <ctype.h>
#include "button.h"
typedef DebouncedButton Button;

#define MAX_BUTTON_COUNT 16
#define MAX_BUF_SIZE 128

/* DEBUG MESSAGES */
#if 1 // set to "1" to enable printed messages, "0" to disable
#define println(x) Serial.println(x)
#define print(x)   Serial.print(x)
#else
#define println(x)
#define print(x)
#endif

/* ELECTROMAGNET SETTINGS */
#define MAG_COUNT 6
#define PIN_MAG_FIRST 2
#define PIN_MAG_MAN_TOG 24

/* MOTOR SETTINGS */
#define PIN_MOT_PUL 40
#define PIN_MOT_DIR 41
#define PIN_MOT_MAN_REV 22
#define PIN_MOT_MAN_FWD 23
#define MOT_LINEAR_MMperS 55
#define MOT_PULSE_PER_REV 1600
#define MOT_MM_PER_REV 10

/* CALCULATED PARAMS (DO NOT MODIFY) */
#define MOT_DEGREES_PER_PULSE (360.0/MOT_PULSE_PER_REV)
#define MOT_MM_PER_PULSE ((double)MOT_MM_PER_REV/MOT_PULSE_PER_REV)
#define MOT_PULSE_DUR_MS (1000.0/MOT_LINEAR_MMperS*MOT_MM_PER_PULSE)
/*================================================================*/

/* STATE VARS */
static bool em_state = false;
/*================================================================*/

/* FUNCTIONS */
void processMessage(String); // forwd decl
void processIncomingByte(const byte inByte) {
  static char msgbuf[MAX_BUF_SIZE];
  static int bufpos = 0;

  switch (inByte) {
    case '\n':
      // move buf to msg and clear buf
      msgbuf[bufpos] = 0; // null byte terminator
      bufpos = 0; // "clear" buffer
      processMessage(String(msgbuf));
      break;
    
    case '\r':
      // Discard carriage return
      break;

    default:
      // add to buffer
      if (bufpos < MAX_BUF_SIZE-1) {
        msgbuf[bufpos++] = inByte;
      }
      break;
  }
}

void setMagnet(int num, bool state) {
  if (num < 1 || num > MAG_COUNT) {
    println("setMagnet() pin specifier must be between 1->" + String(MAG_COUNT) + ". Received \"" + String(num) + "\"");
    return;
  }
  digitalWrite(PIN_MAG_FIRST + num - 1, state);
}
void setAllMagnets(bool state) {
  em_state = state;
  for (int ii = 1; ii <= MAG_COUNT; ii++) {
    setMagnet(ii, state);
  }
}

void motorDirFwd() {
  digitalWrite(PIN_MOT_DIR, LOW);
  delayMicroseconds(30);
}
void motorDirRev() {
  digitalWrite(PIN_MOT_DIR, HIGH);
  delayMicroseconds(30);
}
void motorOneStep() {
  digitalWrite(PIN_MOT_PUL, HIGH);
  delayMicroseconds(1000 * MOT_PULSE_DUR_MS / 2.0);
  digitalWrite(PIN_MOT_PUL, LOW);
  delayMicroseconds(1000 * MOT_PULSE_DUR_MS / 2.0);
}
void motorMoveAngleDeg(double deg) {
  int nsteps = int(deg / MOT_DEGREES_PER_PULSE);
  for (int ii = 0; ii < nsteps; ii++) {
    motorOneStep();
  }
}
void motorMoveLinearMM(double mm) {
  int nsteps = int(mm / MOT_MM_PER_PULSE);
  for (int ii = 0; ii < nsteps; ii++) {
    motorOneStep();
  }
}

void demo() {
  double dist_mm = 200; // range of travel
  int dwell_ms = 1000; // how long to hold "treatment" before cycling next mouse

  println("Running demo in...3");
  delay(1000);
  println("...2");
  delay(1000);
  println("...1");
  delay(1000);
  println("...0");

  setAllMagnets(HIGH);
  motorDirFwd();
  motorMoveLinearMM(dist_mm);
  for (int ii = 1; ii <= MAG_COUNT; ii++) {
    if (ii>1) {
      setMagnet(ii-1, HIGH);
    }
    setMagnet(ii, LOW);
    delay(100);
    motorDirRev();
    motorMoveLinearMM(dist_mm);
    // setAllMagnets(LOW);
    delay(dwell_ms);
    //setAllMagnets(HIGH);
    motorDirFwd();
    motorMoveLinearMM(dist_mm);
    delay(100);
  }
  setMagnet(6,HIGH);
  motorDirRev();
  motorMoveLinearMM(dist_mm);
  setAllMagnets(LOW);
}

void show_help() {
    print("\
CONTROL LANGUAGE\n\
================\n\
MAG_<num>_<state>\n\
  desc:     Turn electromagnet(s) ON/OFF\n\
  example:  \"MAG_3_ON\", \"MAG_ALL_OFF\"\n\
  args:\n\
    - num:    must be an integer between 1->" + String(MAG_COUNT) + " or \"ALL\"\n\
    - state:  must be either \"ON\" or \"OFF\"\n\
---\n\
MOT_<dir>\n\
  desc:     Set carriage travel direction for subsequent MOT_ commands\n\
  example:  \"MOT_FWD\", \"MOT_REV\"\n\
  args:\n\
    - dir:  must be either \"FWD\" or \"REV\"\n\
---\n\
MOT_<type>_<disp>\n\
  desc:     Move the carriage a linear distance or rotate the drive shaft by an angle\n\
            The direction assumes the most recent call of the \"MOT_<dir>\" command\n\
            (or \"FWD\" after reset\n\
  example:  \"MOT_LIN_10\", \"MOT_ANG_120\"\n\
  args:\n\
    - type: type of motion. must be either \"LIN\" (linear; mm) or \"ANG\" (angular; degrees)\n\
    - disp: amount of displacement to apply as a floating-point number (units described in <type>)\n\
---\n\
DEMO\n\
  desc:     Run demo procedure. Cycles each cage into then out of the irradiation field, with delay\n\
---\n\
HELP\n\
  desc:     Show this help message\n\
================\n\
");
}
/*================================================================*/

// BUTTONS
Button* registered_btns[MAX_BUTTON_COUNT];
int num_registered_btns = 0;
Button* registerButton(int pin) {
  Button* btn = new Button(pin);
  registered_btns[num_registered_btns] = btn;
  ++num_registered_btns;
  return btn;
}
void updateButtons() {
  for (int ii=0; ii<num_registered_btns; ii++) {
    registered_btns[ii]->update();
  }
}
/*================================================================*/

/* RUNTIME CODE */
void processMessage(String msg) {
  if (!msg.length()) {
    return;
  }

  if (msg.compareTo("DEMO") == 0) {
    println("Running Demo");
    demo();
  } else if (msg.compareTo("HELP") == 0) { 
    show_help();
  } else if (msg.substring(0, 3) == "MAG") {
    msg = msg.substring(4);
    // parse message
    int div = msg.indexOf("_");
    if (div < 0) {
      println("Invalid Message");
      return;
    }

    // interpret message
    String s_num = msg.substring(0, div);
    String s_state = msg.substring(div + 1);

    int num;
    if (isDigit(s_num[0])) {
      num = s_num.toInt();
    } else if (s_num.compareTo("ALL") == 0) {
      num = -1;
    } else {
      println("Invalid magnet number supplied (must be integer between 1->" + String(MAG_COUNT) + ", or \"ALL\"");
      return;
    }

    int state;
    if (s_state.compareTo("ON") == 0) {
      state = HIGH;
    }
    else if (s_state.compareTo("OFF") == 0) {
      state = LOW;
    }
    else {
      println("Invalid magnet state (must be \"ON\" or \"OFF\")");
      return;
    }

    // perform action
    if (num >= 0) {
      // set one pin state
      if (num < 1 || num > MAG_COUNT) {
        println("Invalid magnet number");
        return;
      } else {
        println("Setting magnet #" + String(num) + " to " + (state ? "ON" : "OFF"));
        setMagnet(num, state);
      }
    } else {
      // set all pin states
      println("Setting all magnets to " + String(state ? "ON" : "OFF"));
      setAllMagnets(state);
    }
  } else if (msg.substring(0, 3) == "MOT") {
    msg = msg.substring(4);
    if (msg.compareTo("FWD") == 0) {
      println("Motor direction set to \"FORWARD\"");
      motorDirFwd();
    } else if (msg.compareTo("REV") == 0) {
      println("Motor direction set to \"REVERSE\"");
      motorDirRev();
    } else {
      int div = msg.indexOf("_");
      if (div != 3) {
        println("Invalid Message");
        return;
      }
      String s_type = msg.substring(0,3);

      double disp;
      disp = msg.substring(4).toDouble();
      if (disp<=0) {
        println("Invalid <disp> supplied");
        return;
      }

      if (s_type.compareTo("LIN")==0) {
        println("Moving carriage " + String(disp) + "mm");
        motorMoveLinearMM(disp);
      } else if (s_type.compareTo("ANG")==0) {
        println("Spinning driveshaft " + String(disp) + "deg");
        motorMoveAngleDeg(disp);
      }
    }
  } else {
      println("Invalid Message");
      return;
  }
}

Button* btn_mot_rev = registerButton(PIN_MOT_MAN_REV);
Button* btn_mot_fwd = registerButton(PIN_MOT_MAN_FWD);
Button* btn_mag_tog = registerButton(PIN_MAG_MAN_TOG);
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.setTimeout(10);

  // initialize inputs/outputs
  for (int ii = PIN_MAG_FIRST; ii <= PIN_MAG_FIRST + MAG_COUNT; ii++) {
    pinMode(ii, OUTPUT);
  }
  pinMode(PIN_MOT_PUL, OUTPUT);
  pinMode(PIN_MOT_DIR, OUTPUT);
  motorDirFwd();
  setAllMagnets(LOW);

  show_help();
}

void loop() {

  // Check button states
  updateButtons();
  if (btn_mot_rev->wasJustPressed()) {
    println("Manually moving backward by 1cm");
    motorDirRev();
    motorMoveLinearMM(2);
    btn_mot_rev->update();
    while (btn_mot_rev->isPressed()) {
      motorOneStep();
      btn_mot_rev->update();
    }
  } else if (btn_mot_fwd->wasJustPressed()) {
    println("Manually moving forward by 1cm");
    motorDirFwd();
    motorMoveLinearMM(2);
    while (btn_mot_fwd->isPressed()) {
      motorOneStep();
      btn_mot_fwd->update();
    }
  } else if (btn_mag_tog->wasJustPressed()) {
    println("Toggling all electromagnets \"" + String(!em_state ? "ON" : "OFF") + "\"");
    setAllMagnets(!em_state);
  }

  // interpret serial (text) commands
  while (Serial.available()) {
    processIncomingByte(Serial.read());
  }

}


