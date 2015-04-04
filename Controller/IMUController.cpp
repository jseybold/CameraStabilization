/*
 *
 *	IMUController.cpp
 *
 *	Team: Marit Brocker, Mitch Cook, Alex Wilson, Caleb Disbrow
 *	Creation Date: 2 April 2015
 *	Project: RIT Senior Design 2014-2015 -- Camera Stabilization
 *
 *	Description: Wrapper functions for the MPU6050.
 *                        Provides initialization and polling.
 *
 */
 /***************************** Preprocessor Flags ****************************/
//#define DEBUG
 /***************************** Include Files *********************************/
#include "IMUController.h"

/***************************** Variables *********************************/
uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
bool dmpReady = false;  // set true if DMP init was successful
uint16_t fifoCount;     // count of all bytes currently in FIFO
uint8_t fifoBuffer[64]; // FIFO storage buffer

// uncomment "OUTPUT_READABLE_EULER" if you want to see Euler angles
// (in degrees) calculated from the quaternions coming from the FIFO.
// Note that Euler angles suffer from gimbal lock (for more info, see
// http://en.wikipedia.org/wiki/Gimbal_lock)
//#define OUTPUT_READABLE_EULER

// uncomment "OUTPUT_READABLE_YAWPITCHROLL" if you want to see the yaw/
// pitch/roll angles (in degrees) calculated from the quaternions coming
// from the FIFO. Note this also requires gravity vector calculations.
// Also note that yaw/pitch/roll angles suffer from gimbal lock (for
// more info, see: http://en.wikipedia.org/wiki/Gimbal_lock)
#define OUTPUT_READABLE_YAWPITCHROLL

// orientation/motion vars
Quaternion q;           // [w, x, y, z]         quaternion container
VectorFloat gravity;    // [x, y, z]            gravity vector
float euler[3];         // [psi, theta, phi]    Euler angle container
float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector

float euler_avg[3];
float euler_count = 0;

float ypr_avg[3];
float ypr_count = 0;


/***************************** ISR *********************************/
volatile bool mpuInterrupt = false;     // indicates whether MPU interrupt pin has gone high
void dmpDataReady() {
    mpuInterrupt = true;
}
/**
 *
 * Calculates the movement of each of the 3 three-phase BLDC motors as they 
 * stabilize the platform. Movement is based on a single 6050MPU IMU that 
 * senses the user's movement. This process does not use any error correction. 
 *
 * @param	Pointer to MPU object, on failure may be null
 *
 * @return	Boolean indicating status of MPU connection. 
 *                True - success
 *                False - Failure
 ******************************************************************************/
 bool IMUController::init(){
        // join I2C bus (I2Cdev library doesn't do this automatically)
    #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
        Wire.begin();
        // TWBR = 24; // 400kHz I2C clock (200kHz if CPU is 8MHz)
    #elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
        Fastwire::setup(400, true);
    #endif
    
    #ifdef DEBUG
    // initialize serial communication
    Serial.begin(115200);
    while (!Serial); // wait for Leonardo enumeration, others continue immediately

    // NOTE: 8MHz or slower host processors, like the Teensy @ 3.3v or Ardunio
    // Pro Mini running at 3.3v, cannot handle this baud rate reliably due to
    // the baud timing being too misaligned with processor ticks. You must use
    // 38400 or slower in these cases, or use some kind of external separate
    // crystal solution for the UART timer.
    
    // initialize device
    Serial.println(F("Initializing I2C devices..."));
    mpu.initialize();

    // verify connection
    Serial.println(F("Testing device connections..."));
    Serial.println(mpu.testConnection() ? F("MPU6050 connection successful") : F("MPU6050 connection failed"));

    // wait for ready
    Serial.println(F("\nSend any character to begin DMP programming and demo: "));
    while (Serial.available() && Serial.read()); // empty buffer
    while (!Serial.available());                 // wait for data
    while (Serial.available() && Serial.read()); // empty buffer again

    // load and configure the DMP
    Serial.println(F("Initializing DMP..."));
    devStatus = mpu.dmpInitialize();

    // supply your own gyro offsets here, scaled for min sensitivity
    mpu.setXGyroOffset(220);
    mpu.setYGyroOffset(76);
    mpu.setZGyroOffset(-85);
    mpu.setZAccelOffset(1788); // 1688 factory default for my test chip

    // make sure it worked (returns 0 if so)
    if (devStatus == 0) {
        // turn on the DMP, now that it's ready
        Serial.println(F("Enabling DMP..."));
        mpu.setDMPEnabled(true);

        // enable Arduino interrupt detection
        Serial.println(F("Enabling interrupt detection (Arduino external interrupt 0)..."));
        attachInterrupt(0, dmpDataReady, RISING);
        mpuIntStatus = mpu.getIntStatus();

        // set our DMP Ready flag so the main loop() function knows it's okay to use it
        Serial.println(F("DMP ready! Waiting for first interrupt..."));
        dmpReady = true;

        // get expected DMP packet size for later comparison
        packetSize = mpu.dmpGetFIFOPacketSize();
    } else {
        // ERROR!
        // 1 = initial memory load failed
        // 2 = DMP configuration updates failed
        // (if it's going to break, usually the code will be 1)
        Serial.print(F("DMP Initialization failed (code "));
        Serial.print(devStatus);
        Serial.println(F(")"));
    }
	#else
	mpu.initialize();
	if(!mpu.testConnection()){
		return false;
	}
	devStatus = mpu.dmpInitialize();
	
	// supply your own gyro offsets here, scaled for min sensitivity
    mpu.setXGyroOffset(220);
    mpu.setYGyroOffset(76);
    mpu.setZGyroOffset(-85);
    mpu.setZAccelOffset(1788); // 1688 factory default for my test chip
	
	// make sure it worked (returns 0 if so)
    if (devStatus == 0) {
        // turn on the DMP, now that it's ready
        mpu.setDMPEnabled(true);

        // enable Arduino interrupt detection
        attachInterrupt(0, dmpDataReady, RISING);
        mpuIntStatus = mpu.getIntStatus();

        // get expected DMP packet size for later comparison
        packetSize = mpu.dmpGetFIFOPacketSize();
        
        return true;
    } else {
        return false;
    }
	#endif
}

int* IMUController::poll(){
    int* values;
    
     if (!dmpReady) return values;
    
    digitalWrite(13, false);
    
    if (mpuInterrupt || fifoCount >= packetSize){  
      // reset interrupt flag and get INT_STATUS byte
      mpuInterrupt = false;
      mpuIntStatus = mpu.getIntStatus();
  
      // get current FIFO count
      fifoCount = mpu.getFIFOCount();
  
      // check for overflow (this should never happen unless our code is too inefficient)
      if ((mpuIntStatus & 0x10) || fifoCount == 1024) {
          // reset so we can continue cleanly
          mpu.resetFIFO();
          Serial.println(F("FIFO overflow!"));
  
      // otherwise, check for DMP data ready interrupt (this should happen frequently)
      } else if (mpuIntStatus & 0x02) {
          // wait for correct available data length, should be a VERY short wait
          while (fifoCount < packetSize) fifoCount = mpu.getFIFOCount();
  
          // read a packet from FIFO
          mpu.getFIFOBytes(fifoBuffer, packetSize);
          
          // track FIFO count here in case there is > 1 packet available
          // (this lets us immediately read more without waiting for an interrupt)
          fifoCount -= packetSize;
  
          #ifdef OUTPUT_READABLE_EULER
              // display Euler angles in degrees
              mpu.dmpGetQuaternion(&q, fifoBuffer);
              mpu.dmpGetEuler(euler, &q);
              if(euler_count < 10) {
                euler_avg[0] += euler[0];
                euler_avg[1] += euler[1];
                euler_avg[2] += euler[2];
                euler_count ++;
              } else {
                Serial.print("euler\t");
                Serial.print(euler_avg[0] * 18/M_PI);
                Serial.print("\t");
                Serial.print(euler_avg[1] * 18/M_PI);
                Serial.print("\t");
                Serial.println(euler_avg[2] * 18/M_PI);
                euler_count = 0;
                euler_avg[0] = 0;
                euler_avg[1] = 0;
                euler_avg[2] = 0;
                
                /*
                accept angle, adjust motor
                analogWrite(PWM_output_pin, 191);
	        delay(5000); // 5 seconds
                */
              }
          #endif
  
          #ifdef OUTPUT_READABLE_YAWPITCHROLL
              // display Euler angles in degrees
              mpu.dmpGetQuaternion(&q, fifoBuffer);
              mpu.dmpGetGravity(&gravity, &q);
              mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
              if (ypr_count < 10) {
                ypr_avg[0] += ypr[0];
                ypr_avg[1] += ypr[1];
                ypr_avg[2] += ypr[2];
                ypr_count ++;
              }else{
                Serial.print("ypr\t");
                Serial.print(ypr_avg[0] * 18/M_PI);
                Serial.print("\t");
                Serial.print(ypr_avg[2] * 18/M_PI);
                Serial.print("\t");
                Serial.println(ypr_avg[1] * 18/M_PI);

                ypr_avg[0] = ypr_avg[0] * 18/M_PI;
                ypr_avg[1] = ypr_avg[1] * 18/M_PI;
                ypr_avg[2] = ypr_avg[2] * 18/M_PI;
                
                
                if (ypr_avg[0] < -10 || ypr_avg[0] > 10) {
                    if (ypr_avg[0] < -10){
                        Serial.println("StartX_Forward");
                    }else if (ypr_avg[0] > 10){
                        Serial.println("StartX_Reverse");
                    }
                }else{
                    Serial.println("StartX_Stop");
                }
                
                if (ypr_avg[1] < -10 || ypr_avg[1] > 10) {
                    if (ypr_avg[1] < -10){
                        Serial.println("StartY_Forward");
                    }else if (ypr_avg[1] > 10){
                        Serial.println("StartY_Reverse");
                    }
                }else{
                    Serial.println("StartY_Stop");
                }
                
                if (ypr_avg[2] < -10 || ypr_avg[2] > 10) {
                    if (ypr_avg[2] < -10){
                        Serial.println("StartZ_Forward");
                    }else if (ypr_avg[2] > 10){
                        Serial.println("StartZ_Reverse");
                    }
                }else{
                    Serial.println("StartZ_Stop");
                }
                
                ypr_count = 0;
                ypr_avg[0] = 0;
                ypr_avg[1] = 0;
                ypr_avg[2] = 0;
                
              }
          #endif
      }
    }
}

MPU6050* IMUController::getIMU(){
     return &mpu;   
}