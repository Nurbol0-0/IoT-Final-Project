# IoT-Final-Project
This project is an Arduino-based RFID attendance system designed to record student attendance automatically.
When a student scans an RFID card, the system reads the card UID, displays a welcome message with the user ID on the LCD, activates the buzzer, and saves the attendance record with date and time to an SD card. 
The saved CSV file can be opened in Microsoft Excel as a table. 
Setup requires connecting the RFID module, RTC DS1302, LCD I2C, SD card module, LED, and buzzer to the Arduino Uno.
