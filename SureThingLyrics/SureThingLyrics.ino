#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

// Using the exact constructor and pin mapping from your working sketch
// (If 0x27 doesn't work for your display, you can change it back to 0x20)
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); 

void printKaraoke(const char* line1, const char* line2, int charSpeed, int holdTime) {
  lcd.clear();
  
  lcd.setCursor(0, 0);
  for (int i = 0; line1[i] != '\0' && i < 16; i++) {
    lcd.print(line1[i]);
    delay(charSpeed);
  }
  
  lcd.setCursor(0, 1);
  for (int i = 0; line2[i] != '\0' && i < 16; i++) {
    lcd.print(line2[i]);
    delay(charSpeed);
  }
  
  delay(holdTime);
}

void setup() {
  lcd.begin(16, 2); // Defining 16 columns and 2 rows of lcd display
  lcd.backlight();  // To Power ON the backlight

  lcd.setCursor(0, 0);
  lcd.print("     MIGUEL -     ");
  lcd.setCursor(0, 1);
  lcd.print("   SURE THING   ");
  delay(3000);
}

void loop() {
  // --- REAL TIMING (85 BPM Flow) ---
  // charSpeed = ms per character (faster text = matches quicker vocals)
  // holdTime = ms pause to let the line breathe before the next lyric
  
  printKaraoke("You could bet",  "that, never",       50, 400);
  printKaraoke("gotta sweat",    "that",              50, 500);

  printKaraoke("You could be",   "the lover,",        55, 450);
  printKaraoke("I'll be the",    "fighter, babe",     55, 500);

  printKaraoke("If I'm the blunt", "(uh), you could",  45, 400);
  printKaraoke("be the lighter,", "babe",              45, 400);
  printKaraoke("Fire it up",     "",                  50, 500);

  printKaraoke("Writer, baby,",    "you could be",      50, 450);
  printKaraoke("the quote, (uh)",  "",                  50, 500);

  printKaraoke("If I'm the lyric,", "you could be",     45, 400);
  printKaraoke("the note (uh),",   "record that!",      45, 500);

  printKaraoke("Saint I'm a",      "sinner (uh),",      50, 400);
  printKaraoke("prize I'm a",      "winner (uh)",       50, 500);

  printKaraoke("And it's you,",    "what did I do",     45, 350);
  printKaraoke("to deserve that?", "",                  50, 500);

  printKaraoke("Paper, baby,",     "I'll be the pen",   50, 500);
  printKaraoke("Say that I'm",     "one 'cause 10",     45, 500);
  printKaraoke("Real and",         "not pretend",       55, 600);

  printKaraoke("Even when sky",    "comes falling",     55, 500);
  printKaraoke("Even when sun",    "don't shine",       55, 500);

  printKaraoke("I got faith",      "in you and I",      55, 500);
  printKaraoke("So put your",      "pretty hand in",    50, 450);
  printKaraoke("mine",             "",                  60, 1500);
}