/* Copyright (c) Colorado School of Mines, 2013.*/
/* All rights reserved.                       */



#ifndef CS_FFT_TOOLS_H
#define CS_FFT_TOOLS_H

namespace cseis_geolib {

class csFFTTools {
public:
  static int const FORWARD = 1;
  static int const INVERSE = 2;

  static int const LOWPASS  = 50;
  static int const HIGHPASS = 51;
  static int const RESAMPLE = 52;
  static int const DESIGNATURE = 53;

  static bool Powerof2( int nx, int* m, int* twopm );
  //  static bool fft( int dir, int power_of_two, double *realValues, double *imagValues );
  static bool fft( int dir, int power_of_two, double *realValues, double *imagValues, bool doNormalisation );
  static bool fft_2d( int dir, double** realValues, double** imagValues, int numFFTSamplesX, int numFFTSamplesY );

public:
  /**
   * @param numSamples Number of samples in input data.
   */
  csFFTTools( int numSamples );
  /**
   * @param numSamples Number of samples in input data.
   * @param sampleInt  Sample interval in input data in milliseconds [ms].
   */
  csFFTTools( int numSamples, float sampleInt );
  /**
   * @param numSamplesIn  Number of samples in input data.
   * @param numSamplesOut Number of samples in output data.
   * @param sampleIntIn   Sample interval in input data in milliseconds [ms].
   * @param sampleIntOut  Sample interval in output data in milliseconds [ms].
   */
  csFFTTools( int numSamplesIn, int numSamplesOut, float sampleIntIn, float sampleIntOut );
  ~csFFTTools();

  void setFilter( int order, float cutOffFreqHz, bool outputImpulseResponse );
  void setFilterWavelet( int length );

  void highPass( float* samples, int order, float cutOffFreqHz, bool outputImpulseResponse );
  void lowPass( float* samples, int order, float cutOffFreqHz, bool outputImpulseResponse );

  void resample( float* samples );
  void resample( float* samples, int order, float cutOffFreqHz );

  /**
   * Apply forward FFT transform
   * @param samples          Input data. Must have numInputSamples() number of samples.
   * @param doNormalisation  Perform normalisation?
   */
  bool fft_forward( float const* samples, bool doNormalisation = false );
  /**
   * Apply forward FFT transform. Compute amplitude spectrum.
   * @param samples          Input data. Must have numInputSamples() number of samples.
   * @param ampSpec          Output amplitude spectrum. Array must have numFFTSamples()/2+1 number of samples.
   * @param doNormalisation  Perform normalisation?
   */
  bool fft_forward( float const* samples, float* ampSpec, bool doNormalisation = false );
  /**
   * Apply forward FFT transform. Compute amplitude & phase spectra.
   * @param samples          Input data. Must have myNumSamplesIn number of samples.
   * @param ampSpec          Output amplitude spectrum. Array must have numFFTSamples()/2+1 number of samples.
   * @param phaseSpec        Output phase spectrum. Array must have numFFTSamples()/2+1 number of samples.
   * @param doNormalisation  Perform normalisation?
   */
  bool fft_forward( float const* samples, float* ampSpec, float* phaseSpec, bool doNormalisation = true );

  /**
   * Apply inverse FFT transform from given amplitude & phase spectrum.
   * @param samples      Input data. Consists of either
   *                      a) Concatenated amplitude and phase spectra, numFFTSamples() number of samples, or
   *                      b) Concatenated real and imaginary spectra, 2*numFFTSamples() number of samples.
   * @param fftDataType  Data 'type': Either FX_REAL_IMAG or FX_AMP_PHASE.
   */
  bool fft_inverse( float const* samples, int fftDataType, bool doNormalisation = true );
  /**
   * Apply inverse FFT transform from given amplitude & phase spectrum.
   * @param amplitude  Input amplitude spectrum. Array must consist of numFFTSamples()/2+1 number of samples.
   * @param phase      Input phase spectrum. Array must consist of numFFTSamples()/2+1 number of samples.
   */
  bool fft_inverse( float const* amplitude, float const* phase, bool doNormalisation = true );

  double const* realData() const { return myBufferReal; }
  double const* imagData() const { return myBufferImag; }

  /**
   * @return Number of samples in FFT transform of input data. Equals nearest power of 2 to numInputSamples().
   */
  int numFFTSamples() const { return myNumFFTSamplesIn; }
  
  /**
   * @return Number of samples in input data.
   */
  int numInputSamples() const { return myNumSamplesIn; }
  
protected:
  void filter( float* samples, int filterType );
  void init();
  void setBuffer( float const* samples );
  void convertFromAmpPhase( float const* ampSpec, float const* phaseSpec );
  void convertToAmpPhase( float* ampSpec, float* phaseSpec );

  /// Number of samples in input data
  int myNumSamplesIn;
  /// Number of samples in output data. Only different for resample operation
  int myNumSamplesOut;
  /// Sample interval of input data
  float mySampleIntIn;  // [ms]
  /// Sample interval of output data
  float mySampleIntOut; // [ms]
  /**
   * Number of samples used in FFT transform for input data.
   * This may be the same as myNumSamplesIn except it is the nearest power of 2 (y^2).
   */
  int myNumFFTSamplesIn;
  /// Number of samples used in FFT transform for output data. Required for resample operation
  int myNumFFTSamplesOut;
  int myTwoPowerIn;
  int myTwoPowerOut;
  double* myBufferReal;
  double* myBufferImag;

  double* myFilterWavelet;
  int     myLengthFilterWavelet;

  int   myOrder;
  float myCutOffFreqHz;
  bool  myOutputImpulseResponse;
  bool  myIsFilterWavelet;
};

} // namespace
#endif


