/// @file

#include <iostream>
#include <math.h>
#include "pitch_analyzer.h"
#include <complex>

using namespace std;

/// Name space of UPC
namespace upc {
  void PitchAnalyzer::autocorrelation(const vector<float> &x, vector<float> &r) const {

    for (unsigned int l = 0; l < r.size(); ++l) {
  		/// \TODO Compute the autocorrelation r[l]
      
      /// \DONE Autocorrelació calculada: 
      /// \f[
      /// r[l] = \frac{1}{N} \sum_{n=0}^{N} x[n] \cdot x[n+l]
      /// \f]
      /// 1. Inicialitzem \f$r[l]\f$ a zero
      /// 2. Acumulem el producte de \f$x[n]\f$ per \f$x[n+l]\f$ per \f$l\le <= n < N\f$
      /// 3. Dividim el resultat per \f$N\f$
      
      r[l]=0;
      for (unsigned int n = 0; n < x.size()-l; n++){
        r[l] += x[n]*x[n+l];
      }
      r[l] /= x.size();
    }

    if (r[0] == 0.0F) //to avoid log() and divide zero 
      r[0] = 1e-10; 
  }

  void PitchAnalyzer::amdf(const vector<float> &x, vector<float> &d) const {
    for (unsigned int l = 0; l < d.size(); ++l) {
      d[l] = 0.0F;
      for (unsigned int n = 0; n < x.size() - l; n++)
        d[l] += fabs(x[n] - x[n + l]);
      d[l] /= x.size();
    }
  }



  float PitchAnalyzer::zcr(const std::vector<float> &x) const {
    float zcr = 0.0F;
    for (unsigned int i = 1; i < x.size(); i++) {
        if ((x[i] >= 0) != (x[i-1] >= 0))
            zcr++;
    }
    return zcr / x.size();
  }


  float PitchAnalyzer::compute_cepstrum_pitch(const vector<float> &x) const {
    unsigned int N = x.size();

    // DFT
    vector<complex<float>> X(N);
    for (unsigned int k = 0; k < N; k++) {
      X[k] = 0;
      for (unsigned int n = 0; n < N; n++)
        X[k] += x[n] * exp(complex<float>(0, -2.0F * M_PI * k * n / N));
    }

    // Log-magnitud
    vector<float> logmag(N);
    for (unsigned int k = 0; k < N; k++)
      logmag[k] = log(abs(X[k]) + 1e-10F);

    // IFFT
    vector<float> cep(N);
    for (unsigned int n = 0; n < N; n++) {
      complex<float> sum = 0;
      for (unsigned int k = 0; k < N; k++)
        sum += logmag[k] * exp(complex<float>(0, 2.0F * M_PI * k * n / N));
      cep[n] = sum.real() / N;
    }

    // Buscar pic entre npitch_min i npitch_max
    unsigned int imax = npitch_min;
    for (unsigned int l = npitch_min; l < npitch_max; l++)
      if (cep[l] > cep[imax]) imax = l;

    return (float) samplingFreq / imax;
  }


  void PitchAnalyzer::set_window(Window win_type) {
    if (frameLen == 0)
      return;

    window.resize(frameLen);

    switch (win_type) {
    case HAMMING:
      /// \TODO Implement the Hamming window
      /// \DONE Finestra Hamming calculada:
      /// Hem aprofitat els càlculs fets a les pràctiques anteriors i n'hem copiat les formules.
      /// \f[
      /// w[n] = 0.54 - 0.46 \cdot \cos\left(\frac{2\pi n}{N-1}\right)
      /// \f]
      /// I d'aquesta manera hem pogut configurar la opció de finestra al docopt.
      for (unsigned int i = 0; i < frameLen; i++){
          window[i] = 0.54F - 0.46F * cos(2.0F * M_PI * i / (frameLen - 1));
      }
      break;
    case RECT:
    default:
      window.assign(frameLen, 1);
    }
  }

  void PitchAnalyzer::set_f0_range(float min_F0, float max_F0) {
    npitch_min = (unsigned int) samplingFreq/max_F0;
    if (npitch_min < 2) npitch_min = 2;  // samplingFreq/2

    npitch_max = 1 + (unsigned int) samplingFreq/min_F0;

    //frameLen should include at least 2*T0
    if (npitch_max > frameLen/2) npitch_max = frameLen/2;
  }

  bool PitchAnalyzer::unvoiced(float pot, float r1norm, float rmaxnorm, float zcr) const {
    /// \TODO Implement a rule to decide whether the sound is voiced or not.
    /// * You can use the standard features (pot, r1norm, rmaxnorm),
    ///   or compute and use other ones.
    /// \DONE Regla implementada:
    /// 1. Si la potencia és baixa, el so és no sonor.
    /// 2. Si la normalització del primer pic i del pic màxim és baixa, el so és no sonor.
    /// 3. Si la taxa de creuaments per zero és alta, el so és no sonor (o sord o mut).
    /// 
    if (pot < llindar_pot) {
      return true;
    }
    if (r1norm < llindar_r1norm && rmaxnorm < llindar_rmaxnorm) {
      return true;
    }
    if (zcr > llindar_zcr)
      return true;
    return false;
  }

float PitchAnalyzer::compute_pitch(vector<float> & x) const {
    if (x.size() != frameLen) return -1.0F;

    //Window input frame
    for (unsigned int i=0; i<x.size(); ++i)
      x[i] *= window[i];

    // Cepstrum
    if (method == 2) {
      float lag_f0 = compute_cepstrum_pitch(x);
      vector<float> r(npitch_max);
      autocorrelation(x, r);
      float pot = 10 * log10(r[0]);
      float z = zcr(x);
#if 0
      if (r[0] > 0.0F)
        cout << pot << '\t' << r[1]/r[0] << '\t' << r[0] << endl;
#endif
      if (unvoiced(pot, r[1]/r[0], r[1]/r[0], z))
        return 0;
      return lag_f0;
    }

    // AMDF
    if (method == 1) {
      vector<float> d(npitch_max);
      amdf(x, d);
      vector<float>::const_iterator iD = d.begin() + npitch_min, iDMin = iD;
      for (; iD < d.begin() + npitch_max; ++iD)
        if (*iD < *iDMin) iDMin = iD;
      unsigned int lag = iDMin - d.begin();
      vector<float> r(npitch_max);
      autocorrelation(x, r);
      float pot = 10 * log10(r[0]);
      float z = zcr(x);
#if 0
      if (r[0] > 0.0F)
        cout << pot << '\t' << r[1]/r[0] << '\t' << d[lag] << endl;
#endif
      if (unvoiced(pot, r[1]/r[0], r[1]/r[0], z))
        return 0;
      return (float) samplingFreq / lag;
    }

    // Autocorrelació (method==0 o method==3)
    vector<float> r(npitch_max);

    //Compute correlation
    autocorrelation(x, r);

    vector<float>::const_iterator iR = r.begin(), iRMax = iR;

    /// \TODO
  	/// Find the lag of the maximum value of the autocorrelation away from the origin.
	  /// Choices to set the minimum value of the lag are:
	  ///    - The first negative value of the autocorrelation.
	  ///    - The lag corresponding to the maximum value of the pitch.
	  /// In either case, the lag should not exceed that of the minimum value of the pitch.
    /// \DONE
    /// S'ha trobat el lag del pic màxim de l'autocorrelació, amb les restriccions indicades.
    /// 1. Inicialitzem iRMax al primer valor de r a partir de npitch_min.
    /// 2. Iterem sobre r des de npitch_min fins a npitch_max i actualitzem
    ///    iRMax si trobem un valor més gran.
    /// També ens hem assegurat que el lag no excedeixi el valor del npitch_min.

    iRMax = r.begin() + npitch_min;
    for(iR= iRMax; iR < r.begin() + npitch_max; ++iR){
      if(*iR > *iRMax){
        iRMax =iR;
      }
    }

    // Combinat: autocorrelació + AMDF
    if (method == 3) {
      vector<float> d(npitch_max);
      amdf(x, d);
      vector<float>::const_iterator iD = d.begin() + npitch_min, iDMin = iD;
      for (; iD < d.begin() + npitch_max; ++iD)
        if (*iD < *iDMin) iDMin = iD;
      unsigned int lag_amdf = iDMin - d.begin();
      unsigned int lag_autocorr = iRMax - r.begin();
      iRMax = r.begin() + (lag_autocorr + lag_amdf) / 2;
    }

    unsigned int lag = iRMax - r.begin();

    float pot = 10 * log10(r[0]);
    float z = zcr(x);

    //You can print these (and other) features, look at them using wavesurfer
    //Based on that, implement a rule for unvoiced
    //change to #if 1 and compile
#if 0
    if (r[0] > 0.0F)
      cout << pot << '\t' << r[1]/r[0] << '\t' << r[lag]/r[0] << endl;
#endif
    
    if (unvoiced(pot, r[1]/r[0], r[lag]/r[0], z))
      return 0;
    else
      return (float) samplingFreq/(float) lag;
  }
}
