#ifndef __BEEP_H__
#define __BEEP_H__

#include "mbed.h"

class Beep {
public:
  Beep(PinName pin):_sp(pin){

  }

  void play(const float m){
    _ticker.attach(callback(this, &Beep::tick),1.0/m/2.0);
    wait(0.5f);
    _ticker.detach();
  }

private:
    void tick(void){
        _sp.write(oto);
        oto=!oto;
    }

public:
  // ド
  constexpr static const float mC = 261.626;
  constexpr static const float mD = 293.665;
  constexpr static const float mE = 329.628;
  constexpr static const float mF = 349.228;
  constexpr static const float mG = 391.995;
  constexpr static const float mA = 440.000;
  constexpr static const float mB = 493.883;

private:
  int oto = 0;
  DigitalOut _sp;
  Ticker _ticker;
};


#endif /* #ifndef __BEEP_H__ */
