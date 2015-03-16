
`Colorspace properties <http://www.avisynth.org/VideoInfo>`_
============================================================

In AviSynth v2.58, the colorspace properties are:
::

 // Colorspace properties.
     enum {
       CS_BGR = 1<<28,
       CS_YUV = 1<<29,
       CS_INTERLEAVED = 1<<30,
       CS_PLANAR = 1<<31
     };

     // Specific colorformats
     enum { CS_UNKNOWN = 0,
            CS_BGR24 = 1<<0 | CS_BGR | CS_INTERLEAVED,
            CS_BGR32 = 1<<1 | CS_BGR | CS_INTERLEAVED,
            CS_YUY2  = 1<<2 | CS_YUV | CS_INTERLEAVED,
            CS_YV12  = 1<<3 | CS_YUV | CS_PLANAR,  // y-v-u,
            4:2:0 planar
            CS_I420  = 1<<4 | CS_YUV | CS_PLANAR,  // y-u-v,
            4:2:0 planar
            CS_IYUV  = 1<<4 | CS_YUV | CS_PLANAR,  // same as
            above
     };

Thus CS_YV12 gives for example:
::

 CS_YV12  = 1<<3 | CS_YUV | CS_PLANAR
              = 1000 | 1000.0000.0000.0000.0000.0000.0000.0000
              | 10.0000.0000.0000.0000.0000.0000.0000
              = 1010.0000.0000.0000.0000.0000.0000.1000

Back to :doc:`VideoInfo <VideoInfo>`

$Date: 2010/03/13 14:51:46 $
