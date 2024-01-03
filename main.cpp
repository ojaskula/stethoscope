#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64
#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int audio_in = 26;      

int audio_signal;
int signal_threshold = 0;
int BPM = 0;
int IBI = 0;
int last_Beat_Time = 0;
bool false_IBI = true;
int false_Time = 0;
const int save_IBI_size = 5;
int save_IBI[save_IBI_size];
bool first_beats = true;
int save_IBI_shifter = 0;
int final_IBI = 0;
const int save_signal_size = 10;
int save_signal[save_signal_size];
int save_signal_shifter = 0;

int avg(int* array, int size);
int median(int* array, int size);
void reset();
void manage_signal();
void manage_IBI();
void print_BPM();
void calculate_final_IBI();

void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.setRotation(2);
  display.setTextSize(3);
  display.setTextColor(WHITE);
  analogReadResolution(12);
}

void loop() {

  manage_signal();

  if(audio_signal > signal_threshold) {

    if(false_IBI == true) { //check missed heartbeat flag
      last_Beat_Time = millis();
      false_IBI = false;
    } 
    else {

      IBI = millis() - last_Beat_Time; //calculate time between heartbeats
      
      if(IBI > 1500) false_IBI = true; //check if the heartbeat was missed
      else if( (IBI > 1.2*final_IBI) && (first_beats == false) ) false_IBI = true;
      else if( (IBI > 0.8*final_IBI) && (IBI > 275) ) { //detect heart beat

        manage_IBI();
        last_Beat_Time = millis();
        calculate_final_IBI();
        BPM = 60000/final_IBI;
        false_Time = millis();
      }
    }
  }
  else {
    if( (millis() - false_Time) > 1700) reset(); //if there was no heartbeat for 1.7s
  }

  print_BPM();
  delay(2);
}


int avg(int* array, int size) { //calculate average
  int avg = 0;
  for (int i = 0; i < size; i++) avg += array[i];
  return avg/size;
}

int median(int *array, int size) { //calculate median
  for (int j=0; j<size-1; ++j) {
    int flag = 0;
    for (int i=0; i<size-j-1; ++i) {
      if (array[i] > array[i+1]) {
        int temp = array[i];
        array[i] = array[i+1];
        array[i+1] = temp;
        flag = 1;
      }
    }
    if (flag == 0) break;
  }
  return array[(size/2)+1];
}

void reset() { //reset all variables
  IBI = 0;
  BPM = 0;
  first_beats = true;
  save_IBI_shifter = 0;
  final_IBI = 0;
  for(int i=0; i<save_IBI_size; i++) save_IBI[i] = 0;
}

void manage_signal() {
  audio_signal = abs(2000 - analogRead(audio_in));
  save_signal[save_signal_shifter] = audio_signal;
  if(save_signal_shifter < save_signal_size) save_signal_shifter++;
  else save_signal_shifter = 0;
  signal_threshold = avg(save_signal, save_signal_size) + 25;
}

void manage_IBI() {
  save_IBI[save_IBI_shifter] = IBI;
  if(save_IBI_shifter < 5) save_IBI_shifter++;
  else {
    save_IBI_shifter = 0;
    first_beats = false;
  }
}

void print_BPM() {
  display.clearDisplay();
  display.setCursor(2, 20);
  display.println("BPM " + String(BPM));
  display.display();
}

void calculate_final_IBI() {
  if(first_beats == true) final_IBI = IBI;
  else final_IBI = median(save_IBI, 5);
}