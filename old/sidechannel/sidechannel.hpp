#ifndef SIDECHANNEL_BASE
#define SIDECHANNEL_BASE


class Sidechannel {
public:
  virtual void init();
  virtual void encode(int idx);
  virtual int decode();
};

#endif