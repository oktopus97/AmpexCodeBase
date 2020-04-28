class Pyrometer  {
  private:
    int _function[3] = {31, 32, 33};
    int _input_pin = A0;
  public:
    Pyrometer();
    int getReading();
    int setEmission(int);
};
