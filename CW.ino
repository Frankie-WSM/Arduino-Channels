#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>

Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

enum state {SYNCHRONISATION, WAITING_PRESS, WAITING_RELEASE}; //enum for all states
const long one_second = 1000;
int scrollCount = 0; //counter that measures how far you have scrolled through the available channels
#define red 0x1
#define yellow 0x3
#define green 0x2
#define purple 0x5
#define white 0x7
String channelArr[26];
byte maxArr[26];
byte minArr[26];
byte valueArr[26];
byte recentArr[26][10];

byte upArrow[8] = {B00100, B01110, B11111, B00100, B00100, B00100, B00100, B00000}; //defining arrows for UDCHARS
byte downArrow[8] = {B00000, B00100, B00100, B00100, B00100, B11111, B01110, B00100};
byte none[8] = {B00000, B00000, B00000, B00000, B00000, B00000, B00000, B00000};

//this function is for debugging
void printChannels() {
  for (int i = 0; i < 26; i++) {
    if (channelArr[i] != NULL) {
      Serial.print(channelArr[i] + ", " + maxArr[i] + ", " + minArr[i] + ", " + valueArr[i] + "\n");
      for (int j = 0; j < 10; j++) {
        Serial.print(recentArr[i][j]);
      }
    } else {
      break;
    }
  }
}

void sortChannels() { //bubble sort algorithm to sort the channels in alphabetical order of channel letter
  String temp1;
  byte temp2;
  byte temp3;
  byte temp4;
  for (int i = 0; i < 26; i++) {
    for (int j = 0; j < 25 - i; j++) {
      if (channelArr[j + 1] != NULL) {
        const char* channel1 = channelArr[j].c_str();
        const char* channel2 = channelArr[j + 1].c_str();
        if ((strcmp(channel1, channel2) > 0 && valueArr[j] != NULL && valueArr[j + 1] != NULL) || (valueArr[j] == NULL && valueArr[j + 1] != NULL)) { //sorts channel, max, min and value arrays by the same index
          temp1 = channelArr[j]; // if statement sends channels with values to the start of the array and sorts
          temp2 = maxArr[j];
          temp3 = minArr[j];
          temp4 = valueArr[j];
          channelArr[j] = channelArr[j + 1];
          maxArr[j] = maxArr[j + 1];
          minArr[j] = minArr[j + 1];
          valueArr[j] = valueArr[j + 1];
          channelArr[j + 1] = temp1;
          maxArr[j + 1] = temp2;
          minArr[j + 1] = temp3;
          valueArr[j + 1] = temp4;
        }
      }
    }
  }
  scrollPrint(9);
}

void addChannel(String msgInput) { //adds a channel letter and description to the channel array
  if (msgInput.length() > 16) {
    msgInput = msgInput.substring(1, 16);
  } else {
    msgInput = msgInput.substring(1);
  }
  for (int i = 0; i < 26; i++) {
    if (channelArr[i].substring(0, 1) == msgInput.substring(0, 1)) { //existing channel updates description
      channelArr[i] = msgInput;
      scrollPrint(9);
      break;
    } else { //new channel
      if (channelArr[i] == NULL) {
        channelArr[i] = msgInput; //channel description
        maxArr[i] = 255; //max
        minArr[i] = 0; //min
        sortChannels();
        break;
      }
    }
  }
}

void addValue(String msgInput) { //adds a value for a channel
  for (int i = 0; i < 26; i++) {
    if (channelArr[i].substring(0, 1) == msgInput.substring(1, 2)) {
      valueArr[i] = msgInput.substring(2).toInt();
      if (recentArr[i][9] != NULL) {
        for (int j = 0; j < 9; j++) {
          recentArr[i][j] = recentArr[i][j + 1];
        }
        recentArr[i][9] = msgInput.substring(2).toInt();
      } else {
        for (int j = 0; j < 10; j++) {
          if (recentArr[i][j] == NULL) {
            recentArr[i][j] = msgInput.substring(2).toInt();
            break;
          }
        }
      }
      sortChannels();
      scrollPrint(9);
      break;
    }
  }
}

void addMax(String msgInput) { //adds a maximum value for channel
  for (int i = 0; i < 26; i++) {
    if (channelArr[i].substring(0, 1) == msgInput.substring(1, 2)) {
      maxArr[i] = msgInput.substring(2).toInt();
      break;
    }
  }
}

void addMin(String msgInput) { //adds a minimum value for channel
  for (int i = 0; i < 26; i++) {
    if (channelArr[i].substring(0, 1) == msgInput.substring(1, 2)) {
      minArr[i] = msgInput.substring(2).toInt();
      break;
    }
  }
}

boolean maxCheck () { //iterates through value array checking against max value of same index (channel)
  for (int i = 0; i < 26; i++) {
    if (valueArr[i] > maxArr[i] && valueArr[i] != NULL && channelArr[i] != NULL) {
      return true;
      break;
    }
  }
  return false;
}

boolean minCheck () { //iterates through value array checking against min value of same index (channel)
  for (int i = 0; i < 26; i++) {
    if (valueArr[i] < minArr[i] && valueArr[i] != NULL && channelArr[i] != NULL) {
      return true;
      break;
    }
  }
  return false;
}

void msgCheck(String msgInput) { //function to check input values
  int inputLen = msgInput.length();
  if (msgInput.substring(0, 1) == "C" && msgInput.substring(2) != NULL) {
    addChannel(msgInput);
  } else if (msgInput.substring(0, 1) == "X" && msgInput.substring(2) != NULL && inputLen <= 5) {
    addMax(msgInput);
  } else if (msgInput.substring(0, 1) == "N" && msgInput.substring(2) != NULL && inputLen <= 5) {
    addMin(msgInput);
  } else if (msgInput.substring(0, 1) == "V" && msgInput.substring(2) != NULL && inputLen <= 5) {
    addValue(msgInput);
  } else {
    Serial.print("ERROR: " + msgInput);
  }
  printChannels();
}

void scrollPrint(int pressed) { //scrolling and printing function (takes button number as input)
  lcd.createChar(0, none);
  lcd.createChar(1, upArrow);
  lcd.createChar(2, downArrow);

  if (valueArr[1] == NULL && valueArr[0] != NULL) { //print one channel
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.write(0);
    lcd.setCursor(1, 0);
    printLine(valueArr[0], channelArr[0]);
  } else if (valueArr[2] == NULL && valueArr[0] != NULL && valueArr[1] != NULL) { //print 2 channels
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.write(0);
    lcd.setCursor(1, 0);
    printLine(valueArr[0], channelArr[0]);
    lcd.setCursor(0, 1);
    lcd.write(0);
    lcd.setCursor(1, 1);
    printLine(valueArr[1], channelArr[1]);
  } else if (valueArr[0] != NULL) { //print more than 2 channels (and arrows)
    if (pressed == 4) { //down button
      if (valueArr[scrollCount + 2] != NULL) { //check if you can scroll down
        if (scrollCount < 20) {
          scrollCount += 1;
          lcd.clear();
          if (valueArr[scrollCount + 2] == NULL || scrollCount > 20) {
            lcd.setCursor(0, 0);
            lcd.write(1);
            lcd.setCursor(1, 0);
            printLine(valueArr[scrollCount], channelArr[scrollCount]);
            lcd.setCursor(0, 1);
            lcd.write(0);
            lcd.setCursor(1, 1);
            printLine(valueArr[scrollCount + 1], channelArr[scrollCount + 1]);
          } else {
            lcd.setCursor(0, 0);
            lcd.write(1);
            lcd.setCursor(1, 0);
            printLine(valueArr[scrollCount], channelArr[scrollCount]);
            lcd.setCursor(0, 1);
            lcd.write(2);
            lcd.setCursor(1, 1);
            printLine(valueArr[scrollCount + 1], channelArr[scrollCount + 1]);
          }
        }
      }
    }
    if (pressed == 8) { //up button
      if (scrollCount > 0) { //checks if you can scroll up
        scrollCount -= 1;
        lcd.clear();
        if (scrollCount == 0) {
          lcd.setCursor(0, 0);
          lcd.write(0);
          lcd.setCursor(1, 0);
          printLine(valueArr[scrollCount], channelArr[scrollCount]);
          lcd.setCursor(0, 1);
          if (valueArr[scrollCount + 2] == NULL) {
            lcd.write(0);
            lcd.setCursor(1, 1);
            printLine(valueArr[scrollCount + 1], channelArr[scrollCount + 1]);
          } else {
            lcd.write(2);
            lcd.setCursor(1, 1);
            printLine(valueArr[scrollCount + 1], channelArr[scrollCount + 1]);
          }
        } else {
          lcd.setCursor(0, 0);
          lcd.write(1);
          lcd.setCursor(1, 0);
          printLine(valueArr[scrollCount], channelArr[scrollCount]);
          lcd.setCursor(0, 1);
          if (valueArr[scrollCount + 2] == NULL) {
            lcd.write(0);
            lcd.setCursor(1, 1);
            printLine(valueArr[scrollCount + 1], channelArr[scrollCount + 1]);
          } else {
            lcd.write(2);
            lcd.setCursor(1, 1);
            printLine(valueArr[scrollCount + 1], channelArr[scrollCount + 1]);
          }
        }
      }
    }
    if (pressed == 9) { //updates after input or select released
      if (scrollCount == 0) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.write(0);
        lcd.setCursor(1, 0);
        printLine(valueArr[scrollCount], channelArr[scrollCount]);
        lcd.setCursor(0, 1);
        lcd.write(2);
        lcd.setCursor(1, 1);
        printLine(valueArr[scrollCount + 1], channelArr[scrollCount + 1]);
      } else if (valueArr[scrollCount + 2] == NULL) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.write(1);
        lcd.setCursor(1, 0);
        printLine(valueArr[scrollCount], channelArr[scrollCount]);
        lcd.setCursor(0, 1);
        lcd.write(0);
        lcd.setCursor(1, 1);
        printLine(valueArr[scrollCount + 1], channelArr[scrollCount + 1]);
      } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.write(1);
        lcd.setCursor(1, 0);
        printLine(valueArr[scrollCount], channelArr[scrollCount]);
        lcd.setCursor(0, 1);
        lcd.write(2);
        lcd.setCursor(1, 1);
        printLine(valueArr[scrollCount + 1], channelArr[scrollCount + 1]);
      }
    }
  }
}

void printLine(int printMsg, String channel) { //function that generates a line to print and prints it
  String valPrint;
  String printStr = String(printMsg);
  if (printStr.length() == 3) {
    valPrint = (channel.substring(0, 1) + printStr);
  } else if (printStr.length() == 2) {
    valPrint = (channel.substring(0, 1) + " " + printStr.substring(0, 2)); //also prints channel name for NAMES
  } else {
    valPrint = (channel.substring(0, 1) + "  " + printStr.substring(0, 1));
  }
  byte cAvg = channelAverage(channel.substring(0, 1));
  if (cAvg < 10) {
    lcd.print(valPrint + ",  " + cAvg + " " + channel.substring(1));
  } else if (cAvg < 100) {
    lcd.print(valPrint + ", " + cAvg + " " + channel.substring(1));
  } else {
    lcd.print(valPrint + "," + cAvg + " " + channel.substring(1));
  }
}

void freeMemory () { //functions that prints the amount of free SRAM when select is pressed for FREERAM
  char top;
  extern int __heap_start, *__brkval;
  lcd.print("Free SRAM: " + String((int)&top - (int)__brkval) + "B");
}

void scrollName (int lineNum) { //function to scroll channel names on the screen for SCROLL
  static unsigned int pos = 0;
  static unsigned long now = millis();
  String cName = channelArr[lineNum].substring(1) + " ";
  lcd.print(cName.substring(pos, pos + 16));
  if (millis() - now > 500) {
    now = millis();
    if (++pos > cName.length()) {
      pos = 0;
    }
  }
}

byte channelAverage (String channelLetter) {
  int total = 0;
  byte count = 0;
  byte index;
  for (int i = 0; i < 26; i++) {
    if (channelArr[i].substring(0,1) == channelLetter){
       index = i;
    }
  }
  for (int i = 0; i < 10; i++) {
    if (recentArr[index][i] == NULL) {
      break;  
    } else {
      total += recentArr[index][i];
      count += 1;
    }
  }
  return(round(total / count));
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  lcd.begin(16, 2);
  Serial.setTimeout(100);
}

void loop() {
  // put your main code here, to run repeatedly:
  static enum state s = SYNCHRONISATION;
  static long pressTime;
  static int pressed;
  static int __stacktop;
  static boolean upPressed = false;
  static boolean downPressed = false;

  switch (s) {
    case SYNCHRONISATION:
      lcd.setBacklight(purple);
      Serial.print(F("Q"));
      if (Serial.available()) {
        char inputVal = Serial.read();
        if (inputVal == 'X') {
          lcd.setBacklight(white);
          Serial.println(F("\nUDCHARS, SCROLL, FREERAM, NAMES"));
          s = WAITING_PRESS;
        } else {
          s = SYNCHRONISATION;
        }
      }
      delay(one_second);
      break;
    case WAITING_RELEASE:
      pressed = lcd.readButtons();
      if (pressed == 1) {
        if (millis() - pressTime >= 1000) { //only after 1 second of select being pressed
          lcd.clear();
          lcd.setBacklight(purple);
          lcd.setCursor(0, 0);
          lcd.print("F118143");
          lcd.setCursor(0, 1);
          freeMemory();
          s = WAITING_RELEASE;
          delay(10);
        }
      } else if (pressed == 4) { //assures names scroll if down is pressed
        downPressed = true;

        if (channelArr[scrollCount].substring(1).length() > 6 && valueArr[scrollCount] != NULL) {
          lcd.setCursor(10, 0);
          scrollName(scrollCount);
        }
        if (channelArr[scrollCount + 1].substring(1).length() > 6 && valueArr[scrollCount + 1] != NULL) {
          lcd.setCursor(10, 1);
          scrollName(scrollCount + 1);
        }
        s = WAITING_RELEASE;
      } else if (pressed == 8) { //assures names scroll if up is pressed
        upPressed = true;

        if (channelArr[scrollCount].substring(1).length() > 6 && valueArr[scrollCount] != NULL) {
          lcd.setCursor(10, 0);
          scrollName(scrollCount);
        }
        if (channelArr[scrollCount + 1].substring(1).length() > 6 && valueArr[scrollCount + 1] != NULL) {
          lcd.setCursor(10, 1);
          scrollName(scrollCount + 1);
        }
        s = WAITING_RELEASE;
      } else {
        lcd.clear();
        scrollPrint(9);
        s = WAITING_PRESS;
      }
      break;
    case WAITING_PRESS:
      if (valueArr[2] != NULL) { //deals with any new scroll request
        if (downPressed == true) {
          scrollPrint(4);
          downPressed = false;
        } else if (upPressed == true) {
          scrollPrint(8);
          upPressed = false;
        }
      }
      boolean isMax = maxCheck(); //check min and max and change colour accordingly
      boolean isMin = minCheck();
      if (isMax == true && isMin == true) {
        lcd.setBacklight(yellow);
      } else if (isMax == true) {
        lcd.setBacklight(red);
      } else if (isMin == true) {
        lcd.setBacklight(green);
      } else {
        lcd.setBacklight(white);
      }
      if (Serial.available()) { //check for input
        String inputString = Serial.readString();
        if (inputString.length() > 1) {
          msgCheck(inputString);
          scrollPrint(0);
        }
      }
      pressed = lcd.readButtons();
      if (pressed == 1 || pressed == 8 || pressed == 4) { //check for button press
        pressTime = millis();
        s = WAITING_RELEASE;
      } else {
        s = WAITING_PRESS;
      }
      if (channelArr[scrollCount].substring(1).length() > 6 && valueArr[scrollCount] != NULL) { //scroll names for NAMES
        lcd.setCursor(10, 0);
        scrollName(scrollCount);
      }
      if (channelArr[scrollCount + 1].substring(1).length() > 6 && valueArr[scrollCount + 1] != NULL) {
        lcd.setCursor(10, 1);
        scrollName(scrollCount + 1);
      }
      break;
  }
}
