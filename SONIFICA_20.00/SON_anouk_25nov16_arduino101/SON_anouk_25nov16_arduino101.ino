#define MAX			1024
#define MIN			5
#define SENSOR0		0
#define SENSOR1		1
#define THRESH1		500
#define THRESH2		200
#define FILTER_LEN	10

int state0, state1;
int avg0, avg1, scnt;
int sensor0MAF[FILTER_LEN], sensor1MAF[FILTER_LEN];

int calc_avg(int *);
void init_filters();
void update_state(int, int);

void setup() {
	Serial.begin(9600);
	Serial1.begin(38400);

	init_filters();
	state0 = state1 = -1;
	avg0 = avg1 = scnt = 0;
}

void loop() {
	/* Replace oldest samples */
	int a0 = analogRead(A0);
	int a1 = analogRead(A1);
	sensor0MAF[scnt % FILTER_LEN] = a0;	
	sensor1MAF[scnt % FILTER_LEN] = a1;
	avg0 = calc_avg(sensor0MAF);
	avg1 = calc_avg(sensor1MAF);

	/* DEBUG */
	// prints to serial monitor. Remove to reduce latency
	Serial.print("A0= ");
	Serial.print(a0);
	Serial.print(" ");
	Serial.print("A1= ");
	Serial.print(a1);
	Serial.println(" ");
	/* DEBUG */
	
	if (a0 < MAX && a0 >= THRESH1){
		update_state(SENSOR0, 0);
	} else if(a0 <= THRESH2){
		update_state(SENSOR0, 1);
	} else {
		// No change when not touching potentiometer
		update_state(SENSOR0, state0);
	}

	if (a1 < MAX && a1 >= THRESH1){
		update_state(SENSOR1, 0);
	} else if(a1 <= THRESH2){
		update_state(SENSOR1, 1);
	} else {
		// No change when not touching potentiometer
		update_state(SENSOR1, state1);
	}
}

int calc_avg(int *maf) {
	int res = 0;
	for (int i = 0; i < FILTER_LEN; ++i)
	{
		res += maf[i];
	}

	return (res / FILTER_LEN);
}

void init_filters() {
	for (int i = 0; i < FILTER_LEN; ++i)
	{
		sensor0MAF[i] = 0;
		sensor1MAF[i] = 0;
	}
}

void update_state(int sensorNum, int newState) {
	if (sensorNum == 0) {
		if (newState != state0){
			state0 = newState;
			Serial1.write('A' + newState);
		} else {
			// Halt if no state change
			Serial1.write('H');
		}
	}
	else {
		if (newState != state1){
			state1 = newState;
			Serial1.write('E' + newState);
		} else {
			// Halt if no state change
			Serial1.write('H');
		}
	}
}
