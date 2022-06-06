#ifndef FRAME_H
#define FRAME_H

template<unsigned int N>
class Frame
{
public:

   float mValue[N];
   float mInSlope[N];
   float mOutSlope[N];
   float mTime;
};

typedef Frame<1> ScalarFrame;
typedef Frame<3> VectorFrame;
typedef Frame<4> QuaternionFrame;

#endif
