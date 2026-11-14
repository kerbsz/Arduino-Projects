
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

//I2C pins declaration
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); 

void setup() 
{
  lcd.begin(16,2);//Defining 16 columns and 2 rows of lcd display
  lcd.backlight();//To Power ON the back light
}

void loop() 
{
  lcd.clear();//Clean the screen
  lcd.setCursor(0,0); //Defining positon to write from first column,first row .
  lcd.print("Arduino"); //You can write 16 Characters per line .
  delay(1000);//Delay used to give a dynamic effect

  lcd.setCursor(0,1);  //Defining positon to write from first column,second row .
  lcd.print("LCD");
  delay(2000); 

  lcd.clear();
  lcd.setCursor(0,0);  //Defining positon to write from first column,second row .
  lcd.print("LEARNING");
  delay(2000); 

  lcd.setCursor(0,1);  //Defining positon to write from first column,second row .
  lcd.print("MADE EASYY!!!!!!");
  delay(2000); 

}