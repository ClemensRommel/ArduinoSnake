#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <SPI.h>

#if defined(ARDUINO_FEATHER_ESP32) // Feather Huzzah32
  #define TFT_CS         14
  #define TFT_RST        15
  #define TFT_DC         32

#elif defined(ESP8266)
  #define TFT_CS         4
  #define TFT_RST        16                                            
  #define TFT_DC         5

#else
  // For the breakout board, you can use any 2 or 3 pins.
  // These pins will also work for the 1.8" TFT shield.
  #define TFT_CS        10
  #define TFT_RST        9 // Or set to -1 and connect to Arduino RESET pin
  #define TFT_DC         8
#endif

// For 1.44" and 1.8" TFT with ST7735 use:
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

void zeichneKarte();
void zeichneSchlange();

void neueFrucht();

void setup() {
  Serial.begin(9600);
  pinMode(7, INPUT);
  pinMode(6, INPUT);
  pinMode(5, INPUT);
  pinMode(4, INPUT);
  pinMode(3, INPUT);
  pinMode(2, INPUT);

  digitalWrite(7, HIGH);
  digitalWrite(6, HIGH);
  digitalWrite(5, HIGH);
  digitalWrite(4, HIGH);
  digitalWrite(3, HIGH);
  digitalWrite(2, HIGH);

  tft.initR(INITR_144GREENTAB); // Init ST7735R chip, green tab

  randomSeed(analogRead(A0));

  zeichneKarte();

  neueFrucht();

  zeichneSchlange();
}

const uint8_t NUMBER_OF_UNITS = 8;
const uint8_t SCREEN_UNIT = 128 / NUMBER_OF_UNITS;
const uint16_t BACKGROUND_COLOR = 0x02a0;


struct Position {
  uint8_t x;
  uint8_t y;
};

struct Position *koerper = nullptr; 
struct Position kopf = {.x = 1, .y = 1};
struct Position frucht;
struct Position freigewordenePosition = {.x = UINT8_MAX, .y = UINT8_MAX}; 
struct Position richtungen[4] = {
  {.x = 1, .y = 0}, 
  {.x = -1, .y = 0}, 
  {.x = 0, .y = 1}, 
  {.x = 0, .y = -1}
  };
// -1 verhält sich wie -1, obwohl die Position unsigned ist,
// da durch Overflow das Ergebnis bei Addition immernoch eins weniger ist

int laengeDesKoerpers = 0;
int koerperKapazitaet = 0;

const int RECHTS = 0;
const int LINKS = 1;
const int UNTEN = 2;
const int OBEN = 3;

int richtung = RECHTS;

const uint8_t ANZAHL_PORTS = 16;
bool istGedrueckt[ANZAHL_PORTS];

void anhaengen(struct Position neuePosition) {
  if(laengeDesKoerpers >= koerperKapazitaet) { // Wenn das array nicht lang genug ist
    // Wird die laenge verdoppelt
    koerper = (Position*) realloc(koerper, (koerperKapazitaet * 2) * sizeof(struct Position));
    koerperKapazitaet *= 2;
  }
  // Am ende neue position einfuegen
  koerper[laengeDesKoerpers] = neuePosition;
  laengeDesKoerpers++; // Laenge erhoehen
}

void ifGedrueckt(uint8_t, void (*a)(bool));

void links(bool);
void rechts(bool);
void oben(bool);
void unten(bool);

bool kollision();
void verloren();

long letzeBewegung = 0;

int highScore = 0;

void stickEingabe() {
  int x = analogRead(A0);
  int y = analogRead(A1);
  Serial.print("x: ");
  Serial.print(x);
  Serial.print(", y: ");
  Serial.println(y);
  if(x > 600) {
    richtung = RECHTS;
  } else if(x < 200) {
    richtung = LINKS;
  } else if(y > 600) {
    richtung = UNTEN;
  } else if(y < 200) {
    richtung = OBEN;
  }
}

bool resetButton() {
  return digitalRead(6) == LOW || digitalRead(7) == LOW;
}

void tasterEingabe() {
  wennGedrueckt(5, LINKS);
  wennGedrueckt(4, OBEN);
  wennGedrueckt(3, RECHTS);
  wennGedrueckt(2, UNTEN);
}

void loop() {
  if(resetButton()) {
    reset();
  }

  stickEingabe();
  tasterEingabe();

  long zeit = millis();

  if(zeit - letzeBewegung >= 335) {
    letzeBewegung = zeit;
    bewegen();
  }

  if(kollision()) {
    verloren();
  } else {
    zeichneSchlange();
  }
}

void bewegen() {
  if(laengeDesKoerpers <= 0) {
    freigewordenePosition = kopf;
  } else {
    freigewordenePosition = koerper[laengeDesKoerpers-1];
  }

  struct Position letztePosition;
  if(laengeDesKoerpers > 0) {
    letztePosition = koerper[laengeDesKoerpers-1];
  } else {
    letztePosition = kopf;
  }
  for(int i = laengeDesKoerpers-1; i > 0; i--) {
    koerper[i] = koerper[i-1];
  }
  if(laengeDesKoerpers>0) {
    koerper[0] = kopf;
  }
  kopf.x += richtungen[richtung].x;
  kopf.y += richtungen[richtung].y;
  kopf.x %= NUMBER_OF_UNITS;
  kopf.y %= NUMBER_OF_UNITS;
  if(frucht.x == kopf.x && frucht.y == kopf.y) {
    neueFrucht();
    anhaengen(letztePosition);
  }
}

void wennGedrueckt(uint8_t port, int r) {
  bool gedrueckt = digitalRead(port) == LOW;
  if(gedrueckt != istGedrueckt[port]) {
    istGedrueckt[port] = gedrueckt;
    richtung = r;
  }
}

void zeichneKarte() {
  tft.fillScreen(BACKGROUND_COLOR);
}

void zeichneSchlange() {
  tft.fillRect(kopf.x * SCREEN_UNIT, kopf.y * SCREEN_UNIT, SCREEN_UNIT, SCREEN_UNIT, ST7735_ORANGE);

  for(int i = 0; i < laengeDesKoerpers; i++) {
    tft.fillRect(koerper[i].x * SCREEN_UNIT, koerper[i].y * SCREEN_UNIT, SCREEN_UNIT, SCREEN_UNIT, ST7735_GREEN);
  }

  if(freigewordenePosition.x == frucht.x && freigewordenePosition.y == frucht.y) {
    tft.fillRect(freigewordenePosition.x * SCREEN_UNIT, freigewordenePosition.y * SCREEN_UNIT, SCREEN_UNIT, SCREEN_UNIT, ST7735_RED);
  } else {
    tft.fillRect(freigewordenePosition.x * SCREEN_UNIT, freigewordenePosition.y * SCREEN_UNIT, SCREEN_UNIT, SCREEN_UNIT, BACKGROUND_COLOR);
  }
}

bool beruehrtSchlange(struct Position position) {
  if(position.x == kopf.x && position.y == kopf.y) {
    return true;
  }
  for(int i = 0; i < laengeDesKoerpers; i++) {
    if(position.x == koerper[i].x && position.y == koerper[i].y) {
      return true;
    }
  }
  return false;
}

void neueFrucht() {
  // Alte Frucht überzeichnen
  tft.fillRect(frucht.x * SCREEN_UNIT, frucht.y * SCREEN_UNIT, SCREEN_UNIT, SCREEN_UNIT, BACKGROUND_COLOR);
  
  struct Position neuePosition;
  do {
    neuePosition.x = random(1, NUMBER_OF_UNITS);
    neuePosition.y = random(1, NUMBER_OF_UNITS);
  // Wenn die neue Position gleich der Position des Kopfes ist, wird eine neue Position generiert
  } while(beruehrtSchlange(neuePosition));
  frucht = neuePosition;

  tft.fillRect(frucht.x * SCREEN_UNIT, frucht.y * SCREEN_UNIT, SCREEN_UNIT, SCREEN_UNIT, ST7735_RED);
}

bool kollision() {
  if(laengeDesKoerpers <= 0) { // Kein koerper, Kollision unmöglich
    return false;
  }

  for(int i = 0; i < laengeDesKoerpers; i++) {
    if(koerper[i].x == kopf.x && koerper[i].y == kopf.y) {
      return true;
    }
  }
  return false;
}

void reset() {
  richtung = RECHTS;
  free(koerper);
  koerper = nullptr;
  kopf = {.x = 1, .y = 1};
  freigewordenePosition = {.x = UINT8_MAX, .y = UINT8_MAX};
  laengeDesKoerpers = 0;
  koerperKapazitaet = 0;

  zeichneKarte();
  neueFrucht();
  zeichneSchlange();
}

void verloren() {
  tft.fillScreen(ST77XX_RED);
  tft.setCursor(0, 0);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.println("Verloren");
  tft.print("Score: ");
  tft.println(laengeDesKoerpers);
  highScore = max(highScore, laengeDesKoerpers);
  tft.print("Highscore:");
  tft.println(highScore);
  while(!resetButton());
  reset();
}