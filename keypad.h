/*
    ECEGRE-2020 - Seattle University

    Description:C++ header file to support 4x3 Keypad

    Author: Tony Tran and John Suizu
*/

#include <thread>

using namespace std;

class Keypad
{

public:
    // Member variables
	char KEYPAD[4][3] = {
        {'1', '2', '3'},
        {'4', '5', '6'},
        {'7', '8', '9'},
        {'*', '0', '#'}
    };
    int COLUMN[3];
    int ROW[4];
	int row_val;
	int col_val;
    
    bool digit_ready;
    bool is_stopped;
    string last_digit;
    
	jthread *m_thread;
	jthread *m_thread2;
	
    // Constructors
    Keypad(void);
    Keypad(int column[3], int row[4]);
    
    // Destructor
    ~Keypad(void);
    
    // public members
    void start_keypad(void);
    
    string get_digit(void);
    
    void get_key(void);

    void exit_keypad(void);

    void stop_keypad(void);
	
	void signalHandler(int signum);
};
