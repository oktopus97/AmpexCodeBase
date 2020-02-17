class GPIOSensor
{
    int calibration_factor;
  public:
    int pin, min1, max1, fixpoint, index;
    void getReadings(int * readings);

    //a method will come to save us all
    //(and do the calibration automatically)
    void setCalibrationFactor(int calib);

    void setFixpoint(int f);

    GPIOSensor(int p, int mi, int ma);
};
