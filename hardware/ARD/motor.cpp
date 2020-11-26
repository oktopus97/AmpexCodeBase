#include "motor.h"
#include "Arduino.h"
MotorDriver::MotorDriver(int _start, int _speed, int _accel, int _ready, int _current) {
	this->_start = _start;
	this->_speed = _speed;
	this->_accel = _accel;
	this->_ready = _ready;
	this->_current = _current;
	pinMode(_start, OUTPUT);
	pinMode(_speed, OUTPUT);
	pinMode(_accel, OUTPUT);
	pinMode(_ready, OUTPUT);
	pinMode(_current, INPUT);
}

void MotorDriver::setSpeed(float speed) { 
	analogWrite(this->_speed, (int)(this->_speed_factor * speed));
}

void MotorDriver::setAccel(float accel) { 
	analogWrite(this->_accel, (int)(this->_accel_factor * accel));
}

void MotorDriver::startSpin(){
	if (this->spinning) return;
	this->spinning = true;
	digitalWrite(_ready, HIGH);
	digitalWrite(_start, HIGH);
}

void MotorDriver::stopSpin(){
	if (!this->spinning) return;
	this->spinning = false;
	digitalWrite(_ready, LOW);
	digitalWrite(_start, LOW);
}

float MotorDriver::getCurrentSpeed() {
       return this->_speed_factor  * analogRead(_current);
}       
