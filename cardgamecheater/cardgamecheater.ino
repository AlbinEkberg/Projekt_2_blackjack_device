dk/*
    File: cardgamecheater.ino
    Author: Albin Ekberg
    Date: 21/2 2024
    Description: A program that checks wether you should hit, stand, split or do something else based on your cards when playing blackjack.
*/
#include "U8glib.h"
#include <Wire.h>

U8GLIB_SSD1306_128X64 oled(U8G_I2C_OPT_NO_ACK);

bool buttonPressed = false; //makes sure the button can not be held down
bool buttonState;
//bool repeatPreviousStage = false; // if this is true the previous stage of the game will play again
bool splittable = false;
const int buttonPin = 2;
const int sensorPin = A2;
int gameStage = 0; //which stage of the game are we in?
int sensorValue = 0; //value of potentiometer
const String nameOfCards[13] = {"Two", "Three", "Four", "Five", "Six", "Seven", "Eight", "Nine", "Ten or Royal", "Ace"}; //names of cards to make it easier for the user
int deckAmount;
int yourHand = 0;
int dealersHand = 0;
int ace = 0;

// the 2d array below is a chart over all possible scenarios where the columns represent the dealers cards and the rows represent your hand
const int BLACKJACKCHART[29][10] PROGMEM =
{ // yourHand:
  {1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, // 8
  {1, 4, 4, 4, 4, 1, 1, 1, 1, 1}, // 9
  {1, 4, 4, 4, 4, 4, 4, 4, 1, 1}, // 10
  {1, 4, 4, 4, 4, 4, 4, 4, 4, 4}, // 11
  {1, 1, 2, 2, 2, 1, 1, 1, 1, 1}, // 12
  {2, 2, 2, 2, 2, 1, 1, 1, 1, 1}, // 13
  {2, 2, 2, 2, 2, 1, 1, 1, 1, 1}, // 14
  {2, 2, 2, 2, 2, 1, 1, 1, 7, 1}, // 15
  {2, 2, 2, 2, 2, 1, 1, 7, 7, 7}, // 16
  {2, 2, 2, 2, 2, 2, 2, 2, 2, 2}, // 17
  {1, 1, 1, 4, 4, 1, 1, 1, 1, 1}, // 2 + 1/11
  {1, 1, 1, 4, 4, 1, 1, 1, 1, 1}, // 3 + 1/11
  {1, 1, 4, 4, 4, 1, 1, 1, 1, 1}, // 4 + 1/11
  {1, 1, 4, 4, 4, 1, 1, 1, 1, 1}, // 5 + 1/11
  {1, 4, 4, 4, 4, 1, 1, 1, 1, 1}, // 6 + 1/11
  {2, 5, 5, 5, 5, 2, 2, 1, 1, 1}, // 7 + 1/11
  {2, 2, 2, 2, 2, 2, 2, 2, 2, 2}, // 8 + 1/11
  {2, 2, 2, 2, 2, 2, 2, 2, 2, 2}, // 9 + 1/11
  {2, 2, 2, 2, 2, 2, 2, 2, 2, 2}, // 10 + 1/11
  {6, 6, 3, 3, 3, 3, 1, 1, 1, 1}, // 2 + 2
  {6, 6, 3, 3, 3, 3, 1, 1, 1, 1}, // 3 + 3
  {1, 1, 1, 6, 6, 1, 1, 1, 1, 1}, // 4 + 4
  {4, 4, 4, 4, 4, 4, 4, 4, 1, 1}, // 5 + 5
  {6, 3, 3, 3, 3, 1, 1, 1, 1, 1}, // 6 + 6
  {3, 3, 3, 3, 3, 3, 1, 1, 1, 1}, // 7 + 7
  {3, 3, 3, 3, 3, 3, 3, 3, 3, 3}, // 8 + 8
  {3, 3, 3, 3, 3, 2, 3, 3, 2, 2}, // 9 + 9
  {2, 2, 2, 2, 2, 2, 2, 2, 2, 2}, // 10 + 10
  {3, 3, 3, 3, 3, 3, 3, 3, 3, 3}, // 1/11 + 1/11
};

/*
    Can be used in order to display the amount of memory left while running
    returns: free_memory;
*/
extern unsigned int __heap_start, *__brkval;
unsigned int freeMemory() {
  unsigned int free_memory;
  if ((unsigned int)__brkval == 0)
    free_memory = ((unsigned int)&free_memory) - ((unsigned int)&__heap_start);
  else
    free_memory = ((unsigned int)&free_memory) - ((unsigned int)__brkval);
  return free_memory;
}


//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//void setup:

void setup()
{
  Serial.begin(9600);
  Wire.begin();
  oled.setFont(u8g_font_helvB08);
  pinMode(buttonPin, INPUT);
  Serial.println("bababoey");
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//void loop:

void loop()
{
  sensorValue = (analogRead(sensorPin) - 50) / 100; //reads the potentiometer and makes the value go from 0 - 9
  buttonState = digitalRead(buttonPin); // reads the button pin and becomes true while the button is pressed

  userInterface();
  nextStage();
  bustChecker();
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//functions:

/*
    Handles the gamestage and increases it when the button is pressed
    returns: void
*/
void nextStage()
{
  if (buttonState == HIGH && buttonPressed != true)
  {
    /*
    if (repeatPreviousStage == true)
    {
      Serial.println("repeatPreviousStage");
      gameStage -= 1;
      repeatPreviousStage = false;
    }
    else
    {
      gameStage += 1;
    }
    */
    gameStage += 1;
    buttonPressed = true;
    Serial.println("gameStage: " + String(gameStage));
  }
  else if (buttonState == LOW)
  {
    buttonPressed = false;
  }
}

/*
    The user interface, which consists of a switch that checks which stage of the game we're in and runs functions accordingly.
    returns: void
*/
void userInterface()
{
  switch (gameStage)
  {
    case 1:
      waitUntilNextButton("Dealers card", "nameOfCards");
      dealersHand = cardHandler(dealersHand);
      Serial.println("Dealers hand: " + String(dealersHand));
      break;

    case 2:
      waitUntilNextButton("First card", "nameOfCards");
      yourHand = 0;
      yourHand += cardHandler(yourHand);
      Serial.println("Your hand: " + String(yourHand));
      break;

    case 3:
      waitUntilNextButton("Second/Latest card", "nameOfCards");
      yourHand += cardHandler(yourHand);
      Serial.println("Your hand: " + String(yourHand));
      break;

    case 4:
      winChance();
      waitUntilNextButton("What to do:", String(winChance()));
      break;

    default:
      Serial.println("resetting to stage 1");
      gameStage = 1;
  }
}

/*
    Checks if your hand has gone over 21 which will result in a loss and therefore the game should not continue
    returns: void
*/
void bustChecker()
{
  aceValueChanger();
  if (yourHand > 21 && ace == 0)
  {
    Serial.println("You went bust with " + String(yourHand));
    gameStage = 1;
  }
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Help functions:

/*
    makes sure the code in the switch only runs once by locking the code in a while loop while still allowing the display, sensorValue and buttonState to update
    parameters: String upperDisplayToUser, this is the text which will be displayed on the top of the screen
                String lowerDisplayToUser, this is the text which will be displayed on the bottom of the screen
    returns: void
*/

void waitUntilNextButton(String upperDisplayToUser, String lowerDisplayToUser)
{
  while (buttonState == LOW || buttonPressed == true)
  {

    if (lowerDisplayToUser == "sensorValue")
    {
      updateOled(upperDisplayToUser, String(sensorValue + 1));
    }
    else if (lowerDisplayToUser == "nameOfCards")
    {
      updateOled(upperDisplayToUser, String(nameOfCards[sensorValue]));
    }
    else
    {
      updateOled(upperDisplayToUser, lowerDisplayToUser);
    }
    sensorValue = (analogRead(sensorPin) - 50) / 100; //reads the potentiometer and makes the value go from 0 - 9

    buttonState = digitalRead(buttonPin);

    // checks if the button has been pressed
    if (buttonState == HIGH && !buttonPressed)
    {
      nextStage();
      break; //exits the loop when the button is pressed
    }

    delay(100); // Delay to avoid checking the button state too rapidly
  }

  while (buttonState == HIGH)
  {
    // waits until the button is released before returning
    buttonState = digitalRead(buttonPin);
    delay(100);
  }
}

/*
    finds out what to do based on the information about the dealers and your hand
    returns: a string which tells you what to do depending on the cards in your and the dealers hand
*/
String winChance()
{
  Serial.print("Free memory before winChance: ");
  Serial.println(freeMemory());

  int whatToDo = 0;

  // if statement below matches your cards according to the blackjack chart
  if (yourHand >= 13 && yourHand <= 21 && ace == 1)
  {
    Serial.println("one ace and something else");
    whatToDo = pgm_read_byte(&BLACKJACKCHART[yourHand - 3][dealersHand - 2]);
  }
  else if (yourHand >= 4 && yourHand <= 20 && splittable == true)
  {
    Serial.println("pair");
    whatToDo = pgm_read_byte(&BLACKJACKCHART[(yourHand / 2) + 17][dealersHand - 2]);
  }
  else if (ace == 2)
  {
    Serial.println("two aces");
    whatToDo = pgm_read_byte(&BLACKJACKCHART[28][dealersHand - 2]);
  }
  else
  {
    Serial.println("two regular cards");
    whatToDo = pgm_read_byte(&BLACKJACKCHART[yourHand - 8][dealersHand - 2]);
  }

  Serial.println("whatToDo: " + String(whatToDo));

  // the switch translates the BLACKJACKCHART so the user can understand it more easily
  switch (whatToDo)
  {
    case 1:
      //repeatPreviousStage = true;
      return "Hit!";
      break;

    case 2:
      return "Stand!";
      break;

    case 3:
      return "Split!";
      break;

    case 4:
      //repeatPreviousStage = true;
      return "Double and Hit!";
      break;

    case 5:
      //repeatPreviousStage = true;
      return "Double or Stand!";
      break;

    case 6:
      return "Split then Double or Hit!";
      break;

    case 7:
      return "Surrender or Hit!";
      break;

    default:
      return "something went wrong";
      break;
  }
}

/*
    ensures the ace switches value from 11 to 1 if you're value is over 21
    returns: void
*/
void aceValueChanger()
{
  if (yourHand >= 21 && ace > 0)
  {
    yourHand -= 10;
    ace -= 1;
  }
}

/*
    Handles the logic behind how many cards are in each hand
    parameters: int affectedHand, this could be either your or the dealers hand
    returns: int affectedHand
*/
int cardHandler(int affectedHand)
{
  if (sensorValue == 9 && gameStage > 1) // if an ace is in your hand the ace value goes up by 1, gameStage > 1 makes sure this value does not increase if the dealer gets an ace
  {
    ace += 1;
  }

  if (affectedHand == sensorValue + 2) // checks to see if you've got pairs
  {
    splittable = true;
  }
  affectedHand = sensorValue + 2;
  //remainingCards[sensorValue] -= 1;
  return affectedHand;
}

/*
    Updates the oled screen
    parameters: String upperText, this will display on the upper half of the screen
                String lowerText, this will display on the lower half of the screen
    returns: void
*/
void updateOled (String upperText, String lowerText) {
  oled.firstPage();
  do {
    oled.drawStr(0, 30, upperText.c_str());
    oled.drawStr(0, 60, lowerText.c_str());
  } while (oled.nextPage());

}
