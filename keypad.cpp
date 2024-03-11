/*
    ECEGRE-2020 - Seattle University

    Description: Homework#4 C++ class file to support 4x3 Keypad assignment

    Author: Tony Tran and John Suizu
*/

#include <iostream>
#include <string>
#include <sstream>
#include <csignal>
#include <thread>
#include <wiringPi.h>

#include "keypad.h"

using namespace std;

	//Constructor
Keypad::Keypad(void) {}

	// Define GPIO PINS
Keypad::Keypad(int* usr_col = nullptr, int* usr_row = nullptr) {
    wiringPiSetupGpio();
	int def_col[3] = {5, 6, 13};
    int def_row[4] = {19, 26, 20, 21};

	//Checking if the user is using the right column/pin 
    if (usr_col != nullptr) {  
        for (int j = 0; j < 3; ++j) {
            if (usr_col[j] < 1 || usr_col[j] > 40) {
                cout << "Invalid column value: " << usr_col[j] << endl;
                for (int j = 0; j < 3; ++j) {
                    COLUMN[j] = def_col[j];
                }
                break;
            } else {
                COLUMN[j] = usr_col[j];
            }
        }
    }
    //Checking if the user is using the right row/pin 
    if (usr_row != nullptr) {  
        for (int i = 0; i < 4; ++i) {
            if (usr_row[i] < 1 || usr_row[i] > 40) {
                cout << "Invalid row value: " << usr_row[i] << endl;
                for (int i = 0; i < 4; ++i) {
                    ROW[i] = def_row[i];
                }
                break;
            } else {
                ROW[i] = usr_row[i];
            }
        }
    } 
	//initializign variables
    digit_ready = false;
    is_stopped = false;
    string last_digit;
}

	//Destructor 
Keypad::~Keypad(void) {}

    // Thread run method, invoked by instance 'start()'
void Keypad::start_keypad()
{
    m_thread = new jthread(&Keypad::get_key, this);
	m_thread2 = new jthread(&Keypad::get_digit, this);
}
	//Get the latest digit pressed. Wait here until digit is depressed
string Keypad::get_digit()
{
    while (!is_stopped) {
        if (digit_ready && last_digit != "n") {
            digit_ready = false;
            return last_digit;
        }
        this_thread::sleep_for(0.1s);
    }
	return last_digit;
}
	//Internal method to set and read the GPIOs
void Keypad::get_key()
{
	while (!is_stopped) {
		for (int j = 0; j < 3; j++) {
			pinMode(COLUMN[j], OUTPUT);
			digitalWrite(COLUMN[j], LOW);
		}
		
		//Set all rows as input
		for (int i= 0; i < 4; i++) {
			pinMode(ROW[i], INPUT);
			pullUpDnControl(ROW[i], PUD_UP);
		}
		
		//Scan rows for pushed key/button
		//A valid key press should set "row_val"  between 0 and 3.
		row_val = -1;
		for (int i = 0; i < 4; i++) {
			if (digitalRead(ROW[i]) == LOW) {
				row_val = i;
			}
		}
		
		//if row_val is not 0 through 3 then no button was pressed and
		//we can exit
		if (row_val < 0 || row_val > 3) {
			last_digit = "n";
			digit_ready = false;
			exit_keypad();
			continue;
		}

		//Convert columns to input
		for (int j = 0; j < 3; j++) {
			pinMode(COLUMN[j], INPUT);
			pullUpDnControl(COLUMN[j], PUD_DOWN);
		}
		
		//Switch the i-th row found from scan to output
		pinMode(ROW[row_val], OUTPUT);
		digitalWrite(ROW[row_val], HIGH);
		
		//Scan columns for still-pushed key/button
		// A valid key press should set "col_val"  between 0 and 2.
		col_val = -1;
		for (int j = 0; j < 3; j++) {
			if (digitalRead(COLUMN[j]) == HIGH) {
				col_val = j;
			}
		}
		
		//if col_val is not 0 through 2 then no button was pressed
		//and we can exit
		if (col_val < 0 || col_val > 2) {
			exit_keypad();
			continue;
		}
		
		// Set the value of the key pressed
		exit_keypad();

		if (!digit_ready && last_digit == "n") {
			digit_ready = true;
			last_digit = KEYPAD[row_val][col_val];
		}
	}
}

	// Reinitialize all rows and columns as input at exit
void Keypad::exit_keypad()
{
    for (int i = 0; i < 4; i++) {
        pinMode(ROW[i], INPUT);
        pullUpDnControl(ROW[i], PUD_UP);
    }
    
    for (int j = 0; j < 3; j++) {
        pinMode(COLUMN[j], INPUT);
        pullUpDnControl(COLUMN[j], PUD_UP);
    }
}

//stop the thread
void Keypad::stop_keypad()
{
    is_stopped = true;
	m_thread->join();
    m_thread2->join();
	cout << "Closing..." << endl;
}