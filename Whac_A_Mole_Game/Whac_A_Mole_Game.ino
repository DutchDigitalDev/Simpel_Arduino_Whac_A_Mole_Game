////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////                                                                                                        //////
//////      Naam: Victor Roestenberg                                                                          //////
//////      Concept Naam: Whac-A-Mole                                                                         //////
//////                                                                                                        //////
//////      Concept beschrijving:                                                   Gebruikte componenten:    //////
//////      In de code is een uitwerking van Whac_A_Mole terug te vinden.           - 7 LEDS                  //////
//////      Er zijn drie levels die met een potmetertje worden aangepast.           - 4 BUTTONS               //////
//////      Hoe moeilijker het level dus groen, geel, rood. Dit betekent            - 1 POTMETER              //////
//////      dat het spel korter de tijd heeft maar wel gaan de led lampjes          - 1 AFSTANDSENSOR         //////
//////      sneller. Ook worden de lampjes donkerder door de afstandsensor          - 1 OLED SCHERMPJE        //////
//////      Dit maakt het spel op afstand wat moeilijker en van dichtbij.           - 1 SERVO                 //////
//////      wat makkelijker. Ook is er een timer, de servo laat dus zien                                      //////
//////      wanneer de tijd op is. Het spel wordt gestart met een klik op                                     //////
//////      een random knop. De OLED display laat het geselecteerde level                                     //////
//////      zien en de aantal punten die de speler heeft behaald in het                                       //////
//////      spel.                                                                                             //////
//////                                                                                                        //////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////
/////Start//Library's//////
///////////////////////////


// Library importeren voor de SERVO
#include <Servo.h>

// library's importeren voor de OLED
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

// OLED scherm benodigde parameters (hulp bron OLED scherm, https://randomnerdtutorials.com/guide-for-oled-display-with-arduino/)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


///////////////////////////
//////Eind//Library's//////
///////////////////////////


///////////////////////////
/////Start//Variabelen/////
///////////////////////////


// Variabelen voor de Led, Button en level Arrays
const int buttonPins[] = { 7, 8, A2, A3 };  // Buttonpins array
const int gameLedPins[] = { 3, 5, 6, 10 };  // GameLeds pins array
const int levelLedPins[] = { 11, 12, 13 };  // LevelLeds Pins array


// Variabelen voor de potValue
int potValue;                               // PotValue variabel
int levelRanges[] = { 0, 341, 682, 1023 };  // Array voor level overgangen
int selectedLevel = 0;                      // variabel voor geselecteerde level

// Variabelen voor de game level instellingen
unsigned long levelGameDurations[] = { 30000, 20000, 10000 };  // Level lengtes in een array level 3 = 30sec, level 2 = 20sec, level 3 = 30 sec
unsigned long levelLedToggleDurations[] = { 1200, 800, 500 };  // Level led intervallen voor level 1, 2 & 3

// Variabelen om game statussen te checken
int targetLED = -1;                                      // Het lampje dat momenteel geselecteerd is
int score = 0;                                           // Speler score tijdens en na het spel
bool buttonPressed[4] = { false, false, false, false };  // array voor het voorkomen van dubbele count van de Speler score
unsigned long lastChangeTime = 0;
unsigned long gameStartTime = 0;                                        // Start van de game tijd op 0 milliseconden
unsigned long currentGameDuration = levelGameDurations[selectedLevel];  // Eind van de game tijd op 30000 milliseconden (speeltijd 30 seconden)

bool gameStarted = false;  // Boolean voor als de game gestart is
bool gameover = false;     // Boolean als de game voorbij is

// Variabelen voor de Distance sensor
const int triggerPin = 2;
const int echoPin = 4;

// Variabelen voor de Servo timer
Servo timerServo;
int servoPosition = 0;  // servo positie variabel start op 0


///////////////////////////
//////Eind//Variabelen/////
///////////////////////////

// benodigde functies aanmaken om aanspraak te maken
void updateLevels();
void displayLevels();
void startGame();
void restartGame();
void gameEnd();
void displayScore();
void updateLevelLeds();

///////////////////////////
///////Start//Setup////////
///////////////////////////


void setup() {
  // Serial communicatie aanzetten voor troubleshooting
  Serial.begin(9600);

  // gameLeds en buttons klaarmaken voor gebruik door OUTPUTS en INPUTS van te maken
  for (int i = 0; i < 4; i++) {
    pinMode(gameLedPins[i], OUTPUT);
    pinMode(buttonPins[i], INPUT);
  }

  // levelLeds klaarmaken door er OUTPUTS van te maken
  for (int i = 0; i < 3; i++) {
    pinMode(levelLedPins[i], OUTPUT);
  }

  // Uitlezen van potmeter
  potValue = analogRead(A1);

  // Het level van de game besluiten door middel van een for loop
  for (int i = 0; i < 3; i++) {
    if (potValue >= levelRanges[i] && potValue < levelRanges[i + 1]) {
      selectedLevel = i;
      break;
    }
  }

  // afstandsensor voorbereiden
  pinMode(triggerPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // random nummer genereren
  randomSeed(analogRead(0));

  // alles voor de OLED display (hulp bron OLED scherm, https://randomnerdtutorials.com/guide-for-oled-display-with-arduino/)
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }

  // OLED display voorbereiding
  display.display();
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);


  // set-up voor de servo als timer
  timerServo.attach(9);  // Selecteer de Servo pin
  timerServo.write(0);   // Zet de timer value op 0

  startGame();
  displayLevels();
}

///////////////////////////
////////Eind//Setup////////
///////////////////////////

///////////////////////////
////////Start//Loop////////
///////////////////////////

void loop() {
  // lees potmetervalue om level aan te passen
  potValue = analogRead(A1);

  // level bepalem door middel van potmeter (for loop om levelranges te checken)
  for (int i = 0; i < 3; i++) {
    if (potValue >= levelRanges[i] && potValue < levelRanges[i + 1]) {
      selectedLevel = i;
      updateLevels();
      break;
    }
  }

  startGame();  //startGame functie activeren zodat de game gestart kan worden

  // Als game gestart wordt worden de benodigde stappen uitgevoerd
  if (gameStarted) {
    // Afstand lezen met de afstandssensor (hulp bron, https://projecthub.arduino.cc/Isaac100/getting-started-with-the-hc-sr04-ultrasonic-sensor-7cabe1)
    unsigned long duration, distance;
    digitalWrite(triggerPin, LOW);
    delayMicroseconds(2);
    digitalWrite(triggerPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(triggerPin, LOW);
    duration = pulseIn(echoPin, HIGH);
    distance = (duration / 2) / 29.1;

    // LED licht veranderen door middel van map functie met afstand
    int brightness = constrain(map(distance, 25, 60, 255, 8), 8, 255);  // Map de afstand om tot felheid
    analogWrite(gameLedPins[targetLED], brightness);                    // Felheid toepassen op LED

    // Overige game tijd uitrekenen
    unsigned long currentGameDuration = levelGameDurations[selectedLevel];    // Spel tijd lengte uit de levelgamedurations array gehaald
    unsigned long elapsedTime = millis() - gameStartTime;                     // Hoeveel tijd is er voorbijgegaan sinds de start van de game
    unsigned long timeRemaining = max(0, currentGameDuration - elapsedTime);  // zorgen dat de overige tijd niet in de min schiet

    // Tijd om mappen tot Servo waarde
    servoPosition = map(timeRemaining, 0, currentGameDuration, 0, 180);

    // Servo positie toepassen
    timerServo.write(servoPosition);

    // Als de game over is worden de volgende stappen uitgevoerd
    if (!gameover) {
      // Checken of spel tijd voorbij is aan de gekozen spel lengte
      if (millis() - gameStartTime >= currentGameDuration) {
        gameEnd();  //game end functie activeren
      }

      // Juiste button ingedrukt met het juiste lampje?
      for (int i = 0; i < 4; i++) {
        if (digitalRead(buttonPins[i]) == HIGH && i == targetLED && !buttonPressed[i]) {
          score++;                  //scoreboard voor gebruiker
          buttonPressed[i] = true;  //checken of de button is ingedrukt om dubbele scores te voorkomen
          Serial.print("Score: ");  // Score in monitor printen
          Serial.println(score);    // Score in monitor printen
          displayScore();           // update de OLED als er een nieuw punt is gehaald

          digitalWrite(gameLedPins[targetLED], LOW);  // Target Led uitzetten voordat er een nieuwe aangaat
          buttonPressed[targetLED] = false;           // Reset the flag for the new LED

          targetLED = random(4);                       // Een nieuwe random led targetten
          digitalWrite(gameLedPins[targetLED], HIGH);  // target led vervolgens aanzetten
          lastChangeTime = millis();                   // led verandering tijd vaststellen
        }
      }

      unsigned long currentGameDuration = levelGameDurations[selectedLevel];
      unsigned long ledToggleDuration = levelLedToggleDurations[selectedLevel];

      // Checken of de led langer is aangeweest dan de ingestelde tijd
      if (targetLED != -1 && millis() - lastChangeTime >= ledToggleDuration) {
        // LED was niet optijd aangeraakt dan uit
        digitalWrite(gameLedPins[targetLED], LOW);

        // Nieuwe random led aanzetten
        targetLED = random(4);
        digitalWrite(gameLedPins[targetLED], HIGH);

        // Tijd van deze led verandering onthouden
        lastChangeTime = millis();
      }
    } else {
      // Game over, wachten om te herstarten
      for (int i = 0; i < 4; i++) {
        if (digitalRead(buttonPins[i]) == HIGH) {
          restartGame();
          break;
        }
      }
    }
  }
}

///////////////////////////
////////Eind///Loop////////
///////////////////////////

///////////////////////////
////Start/Extra/Functies///
///////////////////////////



// Update level functie voor level ledpins
void updateLevels() {
  for (int i = 0; i < 3; i++) {
    if (i == selectedLevel) {
      digitalWrite(levelLedPins[i], HIGH);
    } else {
      digitalWrite(levelLedPins[i], LOW);  // Turn off the other level LEDs
    }
  }
}


// displaylevel functie voor OLED
void displayLevels() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Selected Level: ");
  display.println(selectedLevel);
  display.display();
}


// start game functie en bijbehorende stappen
void startGame() {
  if (!gameStarted) {
    for (int i = 0; i < 4; i++) {
      if (digitalRead(buttonPins[i]) == HIGH) {
        gameStarted = true;
        gameover = false;
        score = 0;
        displayScore();
        gameStartTime = millis();  // Record the game start time
        targetLED = random(4);
        digitalWrite(gameLedPins[targetLED], HIGH);
        Serial.println("Game started!");
        break;
      }
    }
  }
}


// einde game functie en bijbehorende stappen
void gameEnd() {
  gameover = true;
  Serial.print("Game over! Score: ");
  Serial.println(score);
  Serial.println("Press any button to restart the game...");

  // Turn off all the LEDs
  for (int i = 0; i < 4; i++) {
    digitalWrite(gameLedPins[i], LOW);
  }

  // GAMEOVER VOOR OLED SCHERM
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Game Over!");
  display.setCursor(0, 10);
  display.print("Score: ");
  display.print(score);
  display.display();
  delay(4000);
  displayLevels();

  // Alle leds uitzetten
  for (int i = 0; i < 4; i++) {
    digitalWrite(gameLedPins[i], LOW);
  }
}


// restart game functie en bijbehorende stappen
void restartGame() {
  gameover = false;
  gameStartTime = millis();
  targetLED = random(4);
  digitalWrite(gameLedPins[targetLED], HIGH);
  score = 0;
  Serial.println("Game restarted!");
}


// Score display functie voor OLED
void displayScore() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Score: ");
  display.print(score);
  display.display();
}

///////////////////////////
////Eind/Extra/Functies////
///////////////////////////
