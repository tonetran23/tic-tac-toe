/* 
    ECEGR-2020 - Final Project
    Description: Tic-tac-toe Client
    
    Author: Eddy Ferre - ferree@seattleu.edu
    Modified by: Brandon Vu

*/


#include <cstring>
#include <iostream>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string>
#include "keypad.h"

using namespace std;

int moves[9] = {0};
int move_idx = 0;
int column[3] = {5, 6, 13};
int row[4] = {19, 26, 20, 21};
string key;
Keypad dig(column, row);

// Prints the 3x3 grid, checks for tie or win
// Returns true if game is over, false otherwise
bool check_grid(string order, bool &isTie, bool &isWon);

int main()
{
	//local definitions 
	dig.start_keypad(); //initiating key pad method
    string server_ip;
    string user_name;
	string order = "";
	string opponent = "";
    cout << "Enter your name > ";
    cin >> user_name;
    cout << "Server IP > ";
    cin >> server_ip;
    
    // creating socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    // specifying address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(10000);
    serverAddress.sin_addr.s_addr = inet_addr(server_ip.c_str());

    // sending connection request
    connect(sock, (struct sockaddr*)&serverAddress, 
            sizeof(serverAddress)); 

    while(true){

        // sending data
        string message = "READY," + user_name + "\r\n";
        send(sock, message.c_str(), message.length(), 0);
		moves[9] = {0};
        move_idx = 0;
        bool game_over = false;
        bool isTie, isWon;
        cout << "Waiting to be paired..." << endl;
        
        while(true){
            // receiving data
            char buffer[1024] = { 0 };
            recv(sock, buffer, sizeof(buffer), 0);
            cout << "Message from server: " << buffer << endl;
            string buff(buffer);

            // Parse server command
            string del = ",";
            int del_pos = buff.find(del);
            string command = buff.substr(0, del_pos);
            buff.erase(buff.begin(), buff.begin() + del_pos + 1);
            string grid = "";
            string key;
            if (command == "START"){
                del_pos = buff.find(del);
                order = buff.substr(0, del_pos);
                buff.erase(buff.begin(), buff.begin() + del_pos + 1);
                del_pos = buff.find("\r");
                opponent = buff.substr(0, del_pos);

                // My turn?
                if (order == "2"){
                    cout << "Waiting for " << opponent << "'s first move" << endl;
                    continue; // no
                }
                cout << "My first move" << endl;
            }
            else if (command == "PLAY"){
                del_pos = buff.find("\r");
                grid = buff.substr(0, del_pos);
                moves[move_idx++] = stoi(grid);
                game_over = check_grid(order, isTie, isWon);
                if(game_over)
                {
                    if(isTie)
                        cout << "Tied game" << endl;
                    if(!isWon)
                        cout << "Congratulations, " << opponent << " wins"<< endl;
                    break;
                }
            }
            else{ 
                break; //player got error
            }
			
			//checks for valid key press
			bool is_valid = false;
            while (!is_valid){
				cout << "Enter key (1-9) > " << flush;
				// method to get key pad inputs
				key = dig.get_digit();
				cout << key << endl << flush;
				
				//checks for invaild key press
				if(key == "*" || key == "0" || key == "#"){
					cout << "Invalid key pressed \'" << key << "\'" << endl;
					continue;
				}
			//checks for duplicated key press
				int dup_key = stoi(key);
				is_valid = true;
				for (int i=0; i< move_idx; i++){
					if(moves[i] == dup_key){
						is_valid = false;
						cout<< "duplicated key \'" << key << "\'" <<endl;
						break;
					}
				}
			}
			
			
            message = "PLAY," + key + "\r\n";
			cout << "your play: " << key << endl;
            send(sock, message.c_str(), message.length(), 0);
			moves[move_idx++] = stoi(key);
            game_over = check_grid(order, isTie, isWon);
            if(game_over)
            {
                if(isTie)
                    cout << "Tied game" << endl;
                if(isWon)
                    cout << "I won!" << endl;
                break;
            }
        }
    }
    return 0; 
}

bool check_grid(string order, bool &isTie, bool &isWon) {
    char grid[3][3] = {{' ',' ',' '},{' ',' ',' '},{' ',' ',' '}};
    bool my_turn = order == "1";
    bool game_over = false;
	isTie = false;
	isWon = false;
	char winner = '\0';
    for(int i = 0; i < move_idx ; i++)
    {
        switch(moves[i]){
            case 1:
                grid[0][0] = my_turn ? 'O' : 'X';
                break;
            case 2:
                grid[0][1] = my_turn ? 'O' : 'X';
                break;
            case 3:
                grid[0][2] = my_turn ? 'O' : 'X';
                break;
            case 4:
                grid[1][0] = my_turn ? 'O' : 'X';
                break;
            case 5:
                grid[1][1] = my_turn ? 'O' : 'X';
                break;
            case 6:
                grid[1][2] = my_turn ? 'O' : 'X';
                break;
            case 7:
                grid[2][0] = my_turn ? 'O' : 'X';
                break;
            case 8:
                grid[2][1] = my_turn ? 'O' : 'X';
                break;
            case 9:
                grid[2][2] = my_turn ? 'O' : 'X';
                break;
        }
        my_turn = !my_turn;
    }
    for(int i = 0; i < 3 ; i++)
    {
        for(int j = 0; j < 3 ; j++)
        {
            cout << grid[i][j] << " ";
        }
        cout << endl;
    }
    
    // ADD LOGIC HERE TO CHECK THE GRID FOR A TIE, OR A LOSS OR A WIN
    // Set isTie and isWon flags accordingly

	
// Check for a win (row, column)
    for(int i = 0; i < 3; i++) {
        if(grid[i][0] == grid[i][1] && grid[i][1] == grid[i][2] && grid[i][0] != ' ') {
            winner = grid[i][0];
            break;
        } 
		if(grid[0][i] == grid[1][i] && grid[1][i] == grid[2][i] && grid[0][i] != ' ') {
            winner = grid[0][i];
            break;
        }
    }

	// Check for a win (diagonals)
    if(grid[0][0] == grid[1][1] && grid[1][1] == grid[2][2] && grid[0][0] != ' ') {
        winner = grid[1][1];
    } else if (grid[0][2] == grid[1][1] && grid[1][1] == grid[2][0] && grid[0][2] != ' ') {
        winner = grid[1][1];
    }
	//check for isTie
	if (winner == '\0' && move_idx == 9){
		isTie = true;
		game_over = true;
	} 
	//check for win
	if (winner != '\0'){
		isWon = true;
		game_over = true;
		if (winner == 'X') {
			isWon = false;
		}
	}
    return game_over; // Returns true if game is over, false otherwise
}
