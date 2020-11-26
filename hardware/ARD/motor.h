class MotorDriver {
	private:
		int _speed_factor = 1;
		int _accel_factor = 1;
		int _accel;
		int _speed;
		int _start;
		int _ready;
		int _current;
	public:
		MotorDriver(int _start, int _speed, int _accel, int _ready, int _current);
		bool spinning = false;
		void setSpeed(float);
		void setAccel(float);
		void startSpin();
		void stopSpin();
		float getCurrentSpeed();
};
