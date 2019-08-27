#include <Cmd.h>
#include <GPRS_Shield_Arduino.h>

/* *****************************
 *  Pin allocation
 * *****************************
 */
#define LED_pin 13

#define GSM_TX_pin    14
#define GSM_RX_pin    15
#define GSM_POWER_pin 9

#define CMD_pin 10
#define BUZZER_pin 2
#define CONTACT0_pin 3
#define MOVE0_pin 4
#define MOVE1_pin 5
#define MOVE2_pin 6
#define MOVE3_pin 7
#define MOVE4_pin 8
#define MOVE5_pin 9

/* *****************************
 *  Global variables
 * *****************************
 */
GPRS gprs(&Serial3, 19200);
uint32_t gprs_checkPowerUp_task = 0;
uint32_t gprs_checkPowerUp_counter = 0;

#define CMD_NB_MAX 500
uint16_t cmd_nb = 0;
int cmd_current = HIGH;
int cmd_previous = HIGH;
bool alarm_triggered = false;
uint32_t alarm_time = 0;

/* *****************************
 *  Debug Macros
 * *****************************
 */

bool gprs_printIsEnabled  = true;
#define GPRS_PRINT(m)  if(true == gprs_printIsEnabled) { m }

/* *****************************
 *  Command lines funstions
 * *****************************
 */

/* GPRS */
void gprsSendSMS(int arg_cnt, char **args) {
  /* > gprsSendSMS 0612345678 sms_text_to_send_without_space */
  if(3 == arg_cnt) {
    Serial.print("Sending SMS...");
    /* Send the SMS: phoneNumber, text */
    if(true == gprs.sendSMS(args[1], args[2])) { Serial.println("OK"); }
    else { Serial.println("ERROR"); }
  }
  else { Serial.println("Incorrect number of arguments"); }
}
void gprsGetSignalStrength(int arg_cnt, char **args) {
  Serial.print("GPRS signal strength...");
  int signalStrength = 0;
  /* Get signal strength: [0..100] % */
  if(true == gprs.getSignalStrength(&signalStrength)) { Serial.print("OK = "); Serial.print(signalStrength); Serial.println(); } else { Serial.println("ERROR"); }
}
void gprsPowerUpDown(int arg_cnt, char **args) { gprs.powerUpDown(GSM_POWER_pin); Serial.println("GPRS power Up-Down done"); }
void gprsCheckPowerUp(int arg_cnt, char **args) { Serial.print("GPRS check power up..."); if(true == gprs.checkPowerUp()) { Serial.println("OK"); } else { Serial.println("ERROR"); } }
void gprsInit(int arg_cnt, char **args) { Serial.print("GPRS init..."); if(true == gprs.init()) { Serial.println("OK"); } else { Serial.println("ERROR"); } }
void gprsEnablePrint(int arg_cnt, char **args) { gprs_printIsEnabled = true; Serial.println("GPRS print enabled"); }
void gprsDisablePrint(int arg_cnt, char **args) { gprs_printIsEnabled = false; Serial.println("GPRS print disabled"); }

void setup() {
  Serial.begin(115200);
  Serial.println("RF_TRX Starting...");

/* ****************************
 *  Pin configuration
 * ****************************
 */
  pinMode(LED_pin, OUTPUT); 

  pinMode(GSM_TX_pin, OUTPUT);
  digitalWrite(GSM_TX_pin, HIGH);
  pinMode(GSM_RX_pin, INPUT_PULLUP);
  pinMode(GSM_POWER_pin, OUTPUT);
  digitalWrite(GSM_POWER_pin, LOW);

  pinMode(CMD_pin, INPUT_PULLUP);
  pinMode(BUZZER_pin, OUTPUT);
  pinMode(CONTACT0_pin, INPUT_PULLUP);
  pinMode(MOVE0_pin, INPUT_PULLUP);
  pinMode(MOVE1_pin, INPUT_PULLUP);
  pinMode(MOVE2_pin, INPUT_PULLUP);
  pinMode(MOVE3_pin, INPUT_PULLUP);
  pinMode(MOVE4_pin, INPUT_PULLUP);
  pinMode(MOVE5_pin, INPUT_PULLUP);

/* ****************************
 *  Modules initialization
 * ****************************
 */

  gprs.init();

  cmdInit();

  /* ***********************************
   *  Register commands for command line
   * ***********************************
   */
  cmdAdd("gprsSendSMS", "Send SMS", gprsSendSMS);
  cmdAdd("gprsGetSignal", "Get GPRS signal strength", gprsGetSignalStrength);
  cmdAdd("gprsPowerUpDown", "GPRS power up-down", gprsPowerUpDown);
  cmdAdd("gprsCheckPowerUp", "Check GPRS power up", gprsCheckPowerUp);
  cmdAdd("gprsInit", "Initialize GPRS", gprsInit);
  cmdAdd("gprsEnablePrint", "Enable print in GPRS lib", gprsEnablePrint);
  cmdAdd("gprsDisablePrint", "Disable print in GPRS lib", gprsDisablePrint);
  cmdAdd("help", "List commands", cmdList);
  
  Serial.println("Mika Alarm Init done");
}

void loop() {
  if(true == digitalRead(CMD_pin)) {
    if(0 < cmd_nb) { cmd_nb--; }
  }
  else {
    if(cmd_nb < CMD_NB_MAX) { cmd_nb++; }
  }
  if(cmd_nb < CMD_NB_MAX / 2) {
    cmd_current = HIGH;
  }
  else {
    cmd_current = LOW;
  }

  if(LOW == cmd_current) {
    if(HIGH == cmd_previous) {
      digitalWrite(BUZZER_pin, HIGH);
      delay(200);
      digitalWrite(BUZZER_pin, LOW);
    }
    cmd_previous = LOW;
    if((HIGH == digitalRead(CONTACT0_pin)) \
    || (HIGH == digitalRead(MOVE0_pin)) \
    || (HIGH == digitalRead(MOVE1_pin)) \
    || (HIGH == digitalRead(MOVE2_pin)) \
    || (HIGH == digitalRead(MOVE3_pin)) \
    || (HIGH == digitalRead(MOVE4_pin)) \
    || (HIGH == digitalRead(MOVE5_pin))) {
      if(false == alarm_triggered) {
        gprs.sendSMS("0612345678", "Alerte ! Alarme declanchee !");
      }
      alarm_triggered = true;
    }
  }
  else {
    digitalWrite(BUZZER_pin, LOW);
    cmd_previous = HIGH;
    alarm_triggered = false;
  }
  if(true == alarm_triggered) {
    if(alarm_time < 5000) {
      digitalWrite(BUZZER_pin, HIGH);
      alarm_time++;
    }
    else {
      digitalWrite(BUZZER_pin, LOW);
    }
  }
  else {
    digitalWrite(BUZZER_pin, LOW);
    alarm_time = 0;
  }

  gprs_checkPowerUp_task++;
  /* Check GPRS power every 10 seconds */
  if(10000 < gprs_checkPowerUp_task) {
    /* Initialiaze counter for the new cycle */
    gprs_checkPowerUp_task=0;
    /* Power Up cycle */
    gprs_checkPowerUp_counter++;
    /* Get the signal strength and set it in the message */
    /* But also reset the power Up timeout */
    int signalStrengthValue = 0;
    if(true == gprs.getSignalStrength(&signalStrengthValue)) { gprs_checkPowerUp_counter = 0; }
    /* Timeout ! */
    /* We have to turn ON the GPRS shield after 6 attempts */
    if(6 < gprs_checkPowerUp_counter) {
      /* Initialiaze counter for the new cycle */
      gprs_checkPowerUp_counter = 0;
      GPRS_PRINT( Serial.print("Powering up GPRS..."); )
      /* Toggle the power og the GPRS shield */
      gprs.powerUpDown(GSM_POWER_pin);
      GPRS_PRINT( Serial.println("done"); )
      GPRS_PRINT( Serial.print("GPRS init..."); )
      /* Try to initialize thr shield */
      if(true == gprs.init()) { GPRS_PRINT( Serial.println("OK"); ) } else { GPRS_PRINT( Serial.println("ERROR"); ) }
    }
  }

  /* Poll for new command line */
  cmdPoll();
  /* Wait a minimum for cyclic task */
  delay(1);
}