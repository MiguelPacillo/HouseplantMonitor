#include <LiquidCrystal.h>

// LCD Pins

#define rs 7
#define en 8
#define d4 9
#define d5 10
#define d6 11
#define d7 12

// Button ID Numbers

#define leftButton 0
#define midButton 1
#define rightButton 2

// Keeps track of buttons being pressed or held down

int eventArray [3] = {1, 1, 1};
int pressedButton = -1;
int heldButton = -1;

// Speaker Pin

#define speaker 13

// Analog Water Sensor Pin

# define sensor A0

// Contrast value for LCD monitor pin 6

int contrastVal = 70;

// Declare initial setup values

bool reset, mainMenu, settingsMenu, initialCalibrating, finalCalibrating, alarmStart, changingSetting, soundsOn;

int moisturePerc, alarmPerc, initialPerc, currSetting;

double timeToWater, percentTime;

unsigned long startTime, endTime;

LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

void resetValues() {
  reset = true;
  mainMenu = false;
  settingsMenu = false;
  initialCalibrating = true;
  finalCalibrating = true;
  alarmPerc = 5;
  alarmStart = false;
  currSetting = 1;
  changingSetting = false;
  soundsOn = true;
  lcd.clear();
}

void buttonManager() {

  pressedButton = -1; // Here I reset the var for the most recently pressed button

  for (int i = 0; i < 3; i++) {
    eventArray[i] = digitalRead(5 - i); // Gets the states of buttons attached to pin 5, 4, and 3. Where 0 -> Pressed and 1 -> Not pressed

    if (!eventArray[i]) { // If a button is reading 0 it is now being held down
      heldButton = i;
    } else if (eventArray[i] && i == heldButton) { // If the button that was once being held down is now reading 1 (not held down), that button was the last pressed
      heldButton = -1;
      pressedButton = i;

      if (!alarmStart) { // If alarm isn't playing play button tones
        tonePlayer(150, 100);
      }

    }
  }
}

void resetScreen() {

  lcd.setCursor(0, 0);
  lcd.print("Insert sensor");
  lcd.setCursor(0, 1);
  lcd.print("OK to continue");

  if (pressedButton == midButton) {
    readInitialPerc();
    reset = false;
    mainMenu = true;
    lcd.clear();
  }
}

void mainMenuScreen() {

  // Top row displays the water (moisture) percentage in soil

  lcd.setCursor(0, 0);
  lcd.print("Moist(%): ");
  lcd.setCursor(10, 0);
  lcd.print(moisturePerc);
  lcd.setCursor(cursorPlacer(10, moisturePerc), 0);
  lcd.print("                    "); // Add a bunch of spaces here to cover previously used digit places (since display isn't being refreshed every time)

  // Second row displays predicted time before having to water plant again

  lcd.setCursor(0, 1);
  lcd.print("Time(Hr): ");
  lcd.setCursor(10, 1);

  if (finalCalibrating == true) {
    lcd.print("Calib..          ");
  } else {
    lcd.print(timeToWater, 1);
    lcd.setCursor(cursorPlacer(10, timeToWater) + 2, 1);
    lcd.print("                   ");
  }

  if (pressedButton == midButton) {
    mainMenu = false;
    settingsMenu = true;
    lcd.clear();
  }
}



void settingsMenuScreen() {

  if (pressedButton == midButton) { // Toggles the ability to change the selected setting
    if (changingSetting) {
      changingSetting = false;
      lcd.setCursor(0, 1);
      lcd.print("  ");
      lcd.setCursor(14, 1);
      lcd.print("  ");
    } else {
      changingSetting = true;
      lcd.setCursor(0, 1);
      lcd.print(">>");
      lcd.setCursor(14, 1);
      lcd.print("<<");
    }
  }

  if (!changingSetting) { // Allows you to select different settings to change
    if (pressedButton == rightButton && currSetting < 4) {
      currSetting++;
      lcd.clear();
    } else if (pressedButton == leftButton && currSetting > 1) {
      currSetting--;
      lcd.clear();
    }
  }

  switch (currSetting) { // Displays each selected setting

    // Setting 1: Change the percent moisture threshold for when alarm starts to sound
    
    case 1:
      lcd.setCursor(0, 0);
      lcd.print("Alarm Threshold:");
      lcd.setCursor(7, 1);
      lcd.print(alarmPerc);
      lcd.setCursor(cursorPlacer(7, alarmPerc), 1);
      lcd.print("  ");

      if (changingSetting) {
        if (pressedButton == rightButton) {
          alarmPerc++;

          if (alarmPerc > 100) { // So you don't go over 100 percent for alarm
            alarmPerc = 100;
          }
        } else if (pressedButton == leftButton) {
          alarmPerc--;

          if (alarmPerc < 0) { // So you don't go below 0 percent for alarm
            alarmPerc = 0;
          }
        }
      }
      break;

     // Setting 2: Turn system sounds on or off
     
     case 2:
      lcd.setCursor(0, 0);
      lcd.print("System Sounds:");
      lcd.setCursor(7, 1);

      if (changingSetting) {
        if (pressedButton == rightButton) {
          soundsOn = true;
        } else if (pressedButton == leftButton) {
          soundsOn = false;
        }
      }

      if (soundsOn) {
        lcd.print("On ");
      } else {
        lcd.print("Off");
      }
      break;

     // Setting 3: Reset the whole system
 
     case 3:
      lcd.setCursor(0, 0);
      lcd.print("Reset System:");
      lcd.setCursor(7, 1);
      lcd.print("OK");

      if (changingSetting) {
        resetValues();
      }
      break;

     // Setting 4: Exit the settings menu

     case 4:
      lcd.setCursor(0, 0);
      lcd.print("Exit Settings:");
      lcd.setCursor(7, 1);
      lcd.print("OK");

      if (changingSetting) {
        mainMenu = true;
        settingsMenu = false;
        changingSetting = false;
        currSetting = 1;
        lcd.clear();
      }
      break;
  }
  
}

void tonePlayer(int toneFreq, int toneLength) { // Wrapped tone function to disable sounds if the user turns them off
  if (soundsOn) {
    tone(speaker, toneFreq, toneLength);
  }
}

int cursorPlacer(int initialPlace, int value) { // Function for placing cursor to add spaces after a given value being printed

  if (value < 10) {
    return initialPlace + 1;
  } else if (value < 100) {
    return initialPlace + 2;
  } else {
    return initialPlace + 3;
  }
}

void readSensor() { // Here I'll take multiple sensor readings and average them for accuracy

  int maxValue = 400; // This is the maximum value the water sensor can read
  double waterVal = 0;

  for (int i = 0; i < 5; i++) {
    waterVal += analogRead(sensor);
  }

  waterVal = ((waterVal / 5) / maxValue) * 100; // I get the percentage moisture in the soil as waterVal
  moisturePerc = waterVal; // Display waterVal as an int using moisturePerc

}

void readInitialPerc() { // Here I get the initial moisture percentage of the soil so I can later calibrate how much time is needed to water again

  readSensor();
  initialPerc = moisturePerc;
}

void calibrateWateringTime() {

  /*
     To calibrate watering time, I first wait until moisture drops 1 percentage point from the initial percentage (for better accuracy) then start a timer.
     Then I wait for moisture to drop 2 percentage points from the initial and find the amount of time it took to drop an entire percentage point.
     I multiply this time difference by the amount of percentage points left before hitting the alarm percentage
     I then convert the millisecond value to hours and display this in the main menu screen as time before having to water soil again.
  */

  if ((moisturePerc == initialPerc - 1) && initialCalibrating) {

    initialCalibrating = false;
    startTime = millis();
  } else if ((moisturePerc == initialPerc - 2) && finalCalibrating) {

    finalCalibrating = false;
    endTime = millis();

    percentTime = endTime - startTime; // Time (in ms) for moisture to drop 1 percent in soil
  }
}

void alarm() {
  if ((moisturePerc <= alarmPerc) && (!alarmStart)) { // If conditions for the alarm are met and the alarm hasn't started/restarted, turn on alarm
    alarmStart = true;
    startTime = millis();
    endTime = startTime;

    tonePlayer(100, 1000); // Play first tone immediately
  }

  // Once we start the alarm, we wait 4 seconds before restarting it

  if (alarmStart) {
    endTime = millis();

    if ((endTime - startTime) >= 4000) { // Restart alarm after 4 seconds
      alarmStart = false;
    } else if ((endTime - startTime) >= 3000) { // Silence after 3 seconds
      noTone(speaker);
    } else if ((endTime - startTime) >= 2000) { // Play third tone after 2 seconds
      tonePlayer(300, 1000);
    } else if ((endTime - startTime) >= 1000) { // Play second tone after 1 second
      tonePlayer(200, 1000);
    }
  }
}

void setup() {

  analogWrite(6, contrastVal);
  lcd.begin(16, 2);
  resetValues(); // Run reset function in setup to get initial values
}

void loop() {

  buttonManager();
  readSensor();
  calibrateWateringTime();

  // If system is done calibrating, run alarm function and update timeToWater every loop

  if (!finalCalibrating) {
    alarm();
    timeToWater = (percentTime * (moisturePerc - alarmPerc)) / 3600000;
  }

  // Display appropriate menus

  if (reset) {
    resetScreen();
  } else if (mainMenu) {
    mainMenuScreen();
  } else if (settingsMenu) {
    settingsMenuScreen();
  }
}
