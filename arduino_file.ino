#include <Wire.h>
const int LED_PIN =12;
byte gyro_address,gyro_scale=0,mpu_filter,accel_scale=0,accel_filter;
double gyro_sensitivity=131,accel_sensitivity=16384;
int cal_int;
byte gyro_type,choice;

double gyro_roll_cal,gyro_pitch_cal,gyro_yaw_cal;
double temp_variable;
double gyro_roll,gyro_pitch,gyro_yaw;
double accel_x,accel_y,accel_z;
double temp;
float kalman_filter_coeff=0.7;

bool kalman_filter_activated=false;
bool calcul_permitted=false;
byte code_ins,parameter;
long unsigned int timer;

void setup() {
	pinMode(LED_PIN,OUTPUT);
	Wire.begin();                                      //Start the I2C as master
	Serial.begin(115200);
	gyro_address = 0x68;
	//activating
	Wire.beginTransmission(gyro_address);                                      //Start communication with the address found during search.
	Wire.write(0x6B);                                                          //We want to write to the PWR_MGMT_1 register (6B hex)
	Wire.write(0x00);                                                          //Set the register bits as 00000000 to activate the gyro
	Wire.endTransmission();                                                    //End the transmission with the gyro.
	while(Serial.available())Serial.read(); //Clear buffer
	led_blink ();
}

void loop(){
	get_inst();
	if(calcul_permitted){
		timer = micros();
		read_values();
    //appliying sensitivities
		accel_x/=accel_sensitivity;
		accel_y/=accel_sensitivity;
		accel_z/=accel_sensitivity;

		gyro_roll/=gyro_sensitivity;
		gyro_pitch/=gyro_sensitivity;
		gyro_yaw/=gyro_sensitivity;
		print_values();
		while (micros()-timer <50000);
	}
}

void read_values(){
	Wire.beginTransmission(0x68);                                 //Start communication with the gyro (adress 1101001)
	Wire.write(0x3B); 
	Wire.endTransmission();                                      //End the transmission
	Wire.requestFrom(0x68, 14);
	while(Wire.available() < 14);                                 //Wait until the 6 bytes are received
	temp_variable = ((Wire.read()<<8)|Wire.read());                         //Multiply highByte by 256 (shift left by 8) and ad lowByte
	if(!kalman_filter_activated){accel_x=temp_variable;}
	else accel_x=accel_x*kalman_filter_coeff+temp_variable*(1-kalman_filter_coeff);
	temp_variable = ((Wire.read()<<8)|Wire.read());                         //Multiply highByte by 256 (shift left by 8) and ad lowByte
	if(!kalman_filter_activated){accel_y=temp_variable;}
	else accel_y=accel_y*kalman_filter_coeff+temp_variable*(1-kalman_filter_coeff);
	temp_variable = ((Wire.read()<<8)|Wire.read());                         //Multiply highByte by 256 (shift left by 8) and ad lowByte
	if(!kalman_filter_activated){accel_z=temp_variable;}
	else accel_z=accel_z*kalman_filter_coeff+temp_variable*(1-kalman_filter_coeff);
	temp= ((Wire.read()<<8)|Wire.read());

	temp_variable = ((Wire.read()<<8)|Wire.read());                         //Multiply highByte by 256 (shift left by 8) and ad lowByte
	if(cal_int == 2000)temp_variable -= gyro_roll_cal;               //Only compensate after the calibration
	if(!kalman_filter_activated){gyro_roll=temp_variable;}
	else gyro_roll=gyro_roll*kalman_filter_coeff+temp_variable*(1-kalman_filter_coeff);

	temp_variable = ((Wire.read()<<8)|Wire.read());                        //Multiply highByte by 256 (shift left by 8) and ad lowByte
	temp_variable *= -1;                                            //Invert axis
	if(cal_int == 2000)temp_variable -= gyro_pitch_cal;             //Only compensate after the calibration
	if(!kalman_filter_activated){gyro_pitch=temp_variable;}
	else gyro_pitch=gyro_pitch*kalman_filter_coeff+temp_variable*(1-kalman_filter_coeff);

	temp_variable = ((Wire.read()<<8)|Wire.read());                          //Multiply highByte by 256 (shift left by 8) and ad lowByte
	temp_variable *= -1;                                              //Invert axis
	if(cal_int == 2000)temp_variable -= gyro_yaw_cal;                 //Only compensate after the calibration
	if(!kalman_filter_activated){gyro_yaw=temp_variable;}
	else gyro_yaw=gyro_yaw*kalman_filter_coeff+temp_variable*(1-kalman_filter_coeff);
}

void get_inst(){
	read_from_serial(); //Gets instruction and parameter from serial
	if(code_ins!=0)
	{
		if(code_ins==1){
			gyro_type=parameter;
		};
		if(code_ins==2){
			gyro_scale=parameter;
			gyro_sensitivity=131;
			for (int i = 0; i < gyro_scale; ++i)
			{
				gyro_sensitivity=gyro_sensitivity/2;
			}  
			gyro_scale=gyro_scale*8;
	Wire.beginTransmission(gyro_address);                                      //Start communication with the address found during search.
	Wire.write(0x1B);                                                          //We want to write to the GYRO_CONFIG register (1B hex)
	Wire.write(gyro_scale);                                                    //choosing the gyro scale + set FCHOICE_B for mpu6500
	Wire.endTransmission();                                                    //End the transmission with the gyro
	};
	if(code_ins==3){
		accel_scale=parameter;
		accel_sensitivity=16384;
		for (int i = 0; i < accel_scale; ++i)
		{
			accel_sensitivity=accel_sensitivity/2;
		}  
		accel_scale=accel_scale*8;
	Wire.beginTransmission(gyro_address);                                      //Start communication with the address found during search.
	Wire.write(0x1C);                                                          //We want to write to the ACCEL_CONFIG register (1A hex)
	Wire.write(accel_scale);                                                          //Set the register bits as 00010000 (+/- 8g full scale range)
	Wire.endTransmission();                                                    //End the transmission with the gyro
	};
	if(code_ins==4){
		if(gyro_type==0){
			mpu_filter=parameter;
		}
		if(gyro_type==1){
	//reset here the GYRO_CONFIG register (1B hex) to choose FCHOICE_B value 
			if(parameter==0)gyro_scale |= 1 ;
			if(parameter==1)gyro_scale |= 2 ;
			if(parameter==2)mpu_filter = 0 ;
			if(parameter==3)mpu_filter = 1 ;
			if(parameter==4)mpu_filter = 2 ;
			if(parameter==5)mpu_filter = 3 ;
			if(parameter==6)mpu_filter = 4 ;
			if(parameter==7)mpu_filter = 5 ;
			if(parameter==8)mpu_filter = 6 ;
			if(parameter==9)mpu_filter = 7 ;
			Wire.beginTransmission(gyro_address);                                      //Start communication with the address found during search.
			Wire.write(0x1B);                                                          //We want to write to the GYRO_CONFIG register (1B hex)
			Wire.write(gyro_scale);                                                    //choosing the gyro scale + set FCHOICE_B for mpu6500
			Wire.endTransmission();                                                    //End the transmission with the gyro
		}
		Wire.beginTransmission(gyro_address);                                //Start communication with the address found during search
		Wire.write(0x1A);                                            //We want to write to the CONFIG register (1A hex)
		Wire.write(mpu_filter);                                            //Set the register bits as 00000011 (set filter with with 10Hz bandwidth and 17ms delay)
		Wire.endTransmission();                                      //End the transmission with the gyro    
	};
	if(code_ins==5){
		if(gyro_type==1)
		{
			if(parameter==0)accel_filter = 8 ;
			if(parameter==1)accel_filter = 0 ;
			if(parameter==2)accel_filter = 1 ;
			if(parameter==3)accel_filter = 2 ;
			if(parameter==4)accel_filter = 3 ;
			if(parameter==5)accel_filter = 4 ;
			if(parameter==6)accel_filter = 5 ;
			if(parameter==7)accel_filter = 6 ;
		}
		Wire.beginTransmission(gyro_address);                                //Start communication with the address found during search
		Wire.write(0x1D);                                            //We want to write to the ACCEL_CONFIG_2 register (1A hex)
		Wire.write(accel_filter);                                            //Set the register bits as 00000011 (set filter with with 10Hz bandwidth and 17ms delay)
		Wire.endTransmission();                                      //End the transmission with the gyro    
	}
	if(code_ins==6){
		for (cal_int = 0; cal_int < 2000 ; cal_int ++){    //Take 2000 readings for calibration
			read_values();
			gyro_roll_cal += gyro_roll;                        //Ad roll value to gyro_roll_cal.
			gyro_pitch_cal += gyro_pitch;                        //Ad pitch value to gyro_pitch_cal.
			gyro_yaw_cal += gyro_yaw;                        //Ad yaw value to gyro_yaw_cal
			//if(cal_int % 15 == 0)digitalWrite(9, !digitalRead(9));
			delay(4);
			if(cal_int % 20== 0)Serial.write(0x2E);
		}
		//Now that we have 2000 measures, we need to devide by 2000 to get the average gyro offset
		gyro_pitch_cal /= 2000;                                                         //Divide the roll total by 2000.
		gyro_roll_cal /= 2000;                                                         //Divide the pitch total by 2000.
		gyro_yaw_cal /= 2000; 
	};
	if(code_ins==7){
		if(parameter==101){kalman_filter_activated=false;}
		else if (parameter>=0 && parameter<= 100) {
			kalman_filter_activated=true ;kalman_filter_coeff=parameter/100;
		}
	};
	if(code_ins==15){
		calcul_permitted=true;
	};
	if(code_ins==16){
		calcul_permitted=false;
	};
	code_ins=0;
	}
}

void read_from_serial(){
	if(Serial.available()){
		code_ins=Serial.read();
		while(!Serial.available());
		parameter=Serial.read();
		parameter=parameter-1;
		if(code_ins!=0)led_blink();
	}
}

void print_values(){
	Serial.println(gyro_roll);
}

void led_blink () {
	for(int i=0;i<6;i++){
		digitalWrite(LED_PIN,!digitalRead(LED_PIN));
		delay(25);
	}
	digitalWrite(LED_PIN,LOW);
}

