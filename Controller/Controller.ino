#include <U8glib.h>



/*
 *
 *	Controller.ino
 *
 *	Team: Marit Brocker, Mitch Cook, Alex Wilson, Caleb Disbrow
 *	Creation Date: 30 March 2015
 *	Project: RIT Senior Design 2014-2015 -- Camera Stabilization
 *
 *	Description: Main loop for the camera stabilization system
 *
 */
 /***************************** Preprocessor Flags ****************************/
 
#include "PID.h"
#include "customPWM.h"
#include "Variables.h"

const float ANGLE_INIT_THRESHOLD = 0.2;

// declare pins
const int PWM_pin_x = 7;
const int brake_x = 3;
const int enable_x = 2;


const int PWM_pin_y = 8;
const int brake_y = 5;
const int enable_y = 4;

const int PWM_pin_z = 9;
const int brake_z = 10;
const int enable_z = 6;

const int IMU0Interrupt = 48;
const int IMU1Interrupt = 50;

bool base_imu_flag;
bool error_imu_flag;
    
bool PWMisGood = customPWMinit(20000, 100);
customPWM motorPinx(PWM_pin_x);
customPWM motorPiny(PWM_pin_y);
customPWM motorPinz(PWM_pin_z);

// Arduino Wire library is required if I2Cdev I2CDEV_ARDUINO_WIRE implementation
// is used in I2Cdev.h
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    #include "Wire.h"
#endif
#include "LCD_Controller.h"
#include "IMUController.h"

IMUController imu(0);
IMUController imu_error(1);



int* duty;
float* angle_values = (float*) calloc(3,sizeof(float));
float* error_angle_values = (float*) calloc(3,sizeof(float));
float* angle_values_init = (float*) calloc(3,sizeof(float));
float* error_angle_values_init = (float*) calloc(3,sizeof(float));

// ================================================================
// ===                      INITIAL SETUP                       ===
// ================================================================

void setup() {
    Serial.begin(115200);
    
    Wire.begin();
    initialize_LCD();

    // Set pin modes
    // pinMode(brake_x, OUTPUT);
    // pinMode(brake_y, OUTPUT);
    // pinMode(brake_z, OUTPUT);
    
    // pinMode(enable_x, OUTPUT);
    // pinMode(enable_y, OUTPUT);
    // pinMode(enable_z, OUTPUT);
    
    // // Release brakes
    // digitalWrite(brake_x, HIGH);
    // digitalWrite(brake_y, HIGH);
    // digitalWrite(brake_z, HIGH);
    
    // // Disable movement
    // digitalWrite(enable_x, LOW);
    // digitalWrite(enable_y, LOW);
    // digitalWrite(enable_z, LOW);
    
    // attachInterrupt(FWD_BUTT, fwd_butt_handler, HIGH);

    // bool imu_ready = false;
    // bool imu_error_ready = false;
    // bool imu_valid = false;
    // bool imu_error_valid = false;
    
    // imu.init();
    // imu.poll(angle_values);
    
    // imu_error.init();
    // imu_error.poll(error_angle_values);
    
    // while(!imu_ready || !imu_error_ready){
    //   delay(15); // needs small delay for init DO NOT REMOVE >:( 
    //   imu_valid = false;
    //   imu_error_valid = false;
    //   angle_values_init[0] = angle_values[0];
    //   angle_values_init[1] = angle_values[1];
    //   angle_values_init[2] = angle_values[2];
     
    //   error_angle_values_init[0] = error_angle_values[0];
    //   error_angle_values_init[1] = error_angle_values[1];
    //   error_angle_values_init[2] = error_angle_values[2];
      
    //   imu_valid = imu.poll(angle_values);
    //   imu_error_valid = imu_error.poll(error_angle_values);
      
    //   if(imu_valid && !imu_ready){ 
    //     if((abs(angle_values[0] - angle_values_init[0])<ANGLE_INIT_THRESHOLD && abs(angle_values[1] - angle_values_init[1])<ANGLE_INIT_THRESHOLD && abs(angle_values[2] - angle_values_init[2])<ANGLE_INIT_THRESHOLD)){
    //       setBaseAngles(angle_values,0);
    //       imu_ready = true;
    //     }
    //   }
      
    //   if(imu_error_valid && !imu_error_ready){
    //     if((abs(error_angle_values[0] - error_angle_values_init[0])<ANGLE_INIT_THRESHOLD && abs(error_angle_values[1] - error_angle_values_init[1])<ANGLE_INIT_THRESHOLD && abs(error_angle_values[2] - error_angle_values_init[2])<ANGLE_INIT_THRESHOLD)){
    //       setBaseAngles(error_angle_values,1);
    //       imu_error_ready = true;
    //     }
    //   }
    // }
    
    sys_init_complete();
    
    // Enable movement
    // digitalWrite(enable_x, HIGH);
    // digitalWrite(enable_y, HIGH);
    // digitalWrite(enable_z, HIGH);
    // free(angle_values_init);
    // free(error_angle_values_init);
}



// ================================================================
// ===                    MAIN PROGRAM LOOP                     ===
// ================================================================

void loop() {
    //Serial.print("UI: ");
    //Serial.println(in_UI);
    if(in_UI == 0){
        // //get angles from poll
        // base_imu_flag = imu.poll(angle_values);
        // error_imu_flag = imu_error.poll(error_angle_values);

        // //pid returns 3 duty cycles
        // //ignore cases where there was a fifo overflow
        // if( error_imu_flag && base_imu_flag ){
        //     duty = PIDMovementCalc_withError(angle_values, error_angle_values);
      
        //     // debug
        //     Serial.print(F("duty cycles: \t"));                               
        //     Serial.print(duty[0]);
        //     Serial.print(F("\t"));
        //     Serial.print(duty[1]);
        //     Serial.print(F("\t"));
        //     Serial.println(duty[2]);
        //     Serial.println(F(""));
      
        //     //set duty cycles
        //     motorPinx.duty(duty[0]);
        //     motorPiny.duty(duty[1]);
        //     motorPinz.duty(duty[2]);
        
        //     free(duty);
        //     //repeat
        // }
        // else{
        //     Serial.println(F("Something went wrong in retreiving IMU data"));
        // }
    
        in_UI = fwd_butt_handler();
    }else{
      //stop using the IMU, focus on only the LCD interface.
      in_UI = LCD_movement_handler(0);
    }
}


