#include <iostream>
#include <wiringPi.h>
#include <fstream>
#include <string>
#include <istream>
#include <pthread.h>

using namespace std;

//file
const string FILE_STATE = "status_elevator.txt";
const string FILE_DATA = "data_button_press.txt";

//Value of time 
const long red_blinking = 500;

const long floor_arrow = 3000;
const long floor_door = 1000;

//GPIO for the button
const int button_up = 0;
const int button_down = 1;
const int button_reset = 2;
//GPIO for the led
	//Normal up/down led
const int general_white_led = 3;
const int up_led_blue = 4;
const int down_led_green = 5;
	//Error mode
const int error_led_red = 6;

//Matrix for the states
	// S0 -> input state (botton floor
	// S1 -> first floor
	// S2 -> second floor
const int stf[][3] = {{1 ,-1},{2, 0},{-1, 1}};


//function to clear the leds and buttons
void clear_led(){
	
	//leds
	digitalWrite(general_white_led, false);
	digitalWrite(up_led_blue, false);
	digitalWrite(down_led_green, false);
	digitalWrite(error_led_red, false);
	
	}
	
	
//initializing the variable

void init(){
	
	wiringPiSetup();
	
	//button input
	pinMode(button_up, INPUT);
	pinMode(button_down, INPUT);
	pinMode(button_reset, INPUT);
	
	//led output
	pinMode(general_white_led, OUTPUT);
	pinMode(up_led_blue, OUTPUT);
	pinMode(down_led_green,OUTPUT);
	pinMode(error_led_red, OUTPUT);
	
	clear_led();
	
	}
	

void write_data_doc(int number_button_up,int number_button_down, int correct_usage){
	
	ofstream myfile(FILE_DATA);
	if(myfile.is_open()){
		myfile << "Number button up: " << number_button_up << endl;
		myfile << "Number button down: " << number_button_down << endl;
		myfile << "Number actual usage elevator: " << correct_usage << endl;
		myfile.close();
		
		cout << "Saving data on file" << endl;
		}else{
		cout << "Ubable to open file" << endl;
		}
	}
	

int read_state_doc(){
	string line;
	ifstream myfile;
	int value_return = -1;
	
	myfile.open(FILE_STATE);
	if(myfile.is_open()){
		
		while(getline (myfile, line))
		{
			//cout << line << endl;
			char l = line[0];
			value_return = l - 48;
		}
		myfile.close();
		}else {
		cout << "Unable to open file"<< endl;
		}
	
	return value_return;
	}
	
void write_state_doc(int current_state){
	
	ofstream myfile(FILE_STATE);
	if(myfile.is_open()){
		myfile << current_state;
		myfile.close();
		
		cout << "Saving state on file" << endl;
		}else{
		cout << "Ubable to open file" << endl;
		}
	
	
	}
	
void *error_visualization(void *arg){
	
	bool red_on_off = true;
	
	for (int i = 0; i < 4; i++){
		digitalWrite(error_led_red, red_on_off);
		delay(red_blinking);
		red_on_off = !red_on_off;
		}
		
	clear_led();
	return 0;
	}
	
void data_collection(int button, bool correct_instruction){
	
	//statistic value
	static int number_button_up = 0;
	static int number_button_down = 0;
	static int number_usage_elevator = 0;
	
	if(correct_instruction){number_usage_elevator = number_usage_elevator +1;}

	if(button == button_up){
		
		number_button_up = number_button_up +1;
		}else{
		number_button_down =number_button_down +1;
			}
			
	
	cout << "Data Collected!" << endl;
	cout << "Number Usage Buttons: " << number_usage_elevator << endl;
	
	write_data_doc(number_button_up, number_button_down, number_usage_elevator);

	}
	
	
void *controller_arrow(void* value){
	
	int value_led = (int) value;
	
	digitalWrite(value_led, true);
	digitalWrite(general_white_led, true);
	delay(floor_arrow);
	digitalWrite(value_led, false);
	delay(floor_door);
	digitalWrite(general_white_led, false);
	return 0;
	}
	
	
int move_state(int state, int button_value){
	
	
	pthread_attr_t myattr;
	pthread_attr_init(&myattr);
	pthread_t mythread;
	
	void *returnvalue;
	

	int value;
	value = down_led_green;
	

	if(stf[state][button_value] == -1 ){
		
		pthread_create(&mythread, &myattr, error_visualization, NULL);
		
		data_collection(button_value, false);
		
		cout << "Invalid Button, current state: " << state << endl;
		
		cout << "Try again!" << endl;
		
		pthread_join(mythread, &returnvalue);
		
		return state;
	}else{

		
		cout<<"Going to " << stf[state][button_value] << " floor." << endl;
		if(state < stf[state][button_value]){
			value = up_led_blue;
			}
		
		pthread_create(&mythread, &myattr, controller_arrow, (void*) value);
		data_collection(button_value, true);
		pthread_join(mythread, &returnvalue);
		
		
		return stf[state][button_value];
		}
	
	
	}

int main(){

	
	//Initializing the leds and buttons
	init();
	

	cout << "Starting elevator FSM, starting the elevator!" << endl;
	
	int current_state = read_state_doc();
	
	if (current_state< 0 || current_state > 3){
		
		pthread_attr_t myattr;
		pthread_attr_init(&myattr);
		pthread_t mythread;
		void *returnvalue;

		pthread_create(&mythread, &myattr, error_visualization, NULL);
		
		cout << "Error in the: " << FILE_STATE << "file, invalid value: " << current_state << endl;
		
		pthread_join(mythread, &returnvalue);
		
		
		return -1;
		}
	
	cout << "The current state value is:" << current_state << endl;
	
	while(true){
		
		
		//I would have used a callback whenever a button would change the value
		//Button up
		if(digitalRead(button_up) == 1){
			cout << "Button UP press, current state: " << current_state << endl;
			current_state = move_state(current_state, button_up);
			}
			
		if(digitalRead(button_down) == 1){
			cout << "Button DOWN press, current state: " << current_state << endl;
			current_state = move_state(current_state, button_down);
			}
			
		if(digitalRead(button_reset) ==1){
			write_state_doc(current_state);
			cout << "The elevator is OFF! Current State: "<< current_state << endl;
			return 0;
			}
			
		if(digitalRead(button_up) == 1 && digitalRead(button_down) == 1){
			error_visualization(NULL);
			cout << "Error, the button are both press. Try again! Current State:" << current_state << endl;
			}
	
		clear_led();
		}
	
			
			
}



