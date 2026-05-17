/// @file

#include <iostream>
#include <fstream>
#include <string.h>
#include <errno.h>
#include <math.h>

#include "wavfile_mono.h"
#include "pitch_analyzer.h"

#include "docopt.h"

#define FRAME_LEN   0.030 /* 30 ms. */
#define FRAME_SHIFT 0.015 /* 15 ms. */

using namespace std;
using namespace upc;

static const char USAGE[] = R"(
get_pitch - Pitch Estimator 

Usage:
    get_pitch [options] <input-wav> <output-txt>
    get_pitch (-h | --help)
    get_pitch --version

Options:
    -p, --potencia=<val>    Llindar de decisió per a la potència (en dB). [default: -48]
    -1, --r1norm=<val>      Llindar de correlació de 1 per la decisió sonor/sord. [default: 0.95]
    -M, --rmaxnorm=<val>    Llindar de correlació al max secundari per la decisió. [default: 0.38]
    -z, --zcr=<val>         Llindar de ZCR per la decisió sonor/sord. [default: 0.24]
    -w, --window=<val>      Tipus de finestra (0=RECT, 1=HAMMING). [default: 1]
    -f, --minf0=<val>       Freqüència mínima de pitch (Hz). [default: 50]
    -F, --maxf0=<val>       Freqüència màxima de pitch (Hz). [default: 500]
    -m, --median=<val>      Longitud del filtre de mediana (0=sense filtre). [default: 15]
    -c, --clip=<val>        Llindar del central clipping (0=sense clipping). [default: 0.3]
    -t, --method=<val>      Metode: 0=autocorr, 1=amdf, 2=cepstrum, 3=combinat. [default: 0]
    -h, --help              Show this screen
    --version               Show the version of the project


Arguments:
    input-wav   Wave file with the audio signal
    output-txt  Output file: ASCII file with the result of the estimation:
                    - One line per frame with the estimated f0
                    - If considered unvoiced, f0 must be set to f0 = 0
)";

int main(int argc, const char *argv[]) {
	/// \TODO 
	/// Modify the program syntax and the call to **docopt()** in order to
	/// add options and arguments to the program.
  
  /// \DONE 
  /// S'han afegit les opcions i arguments següents:
  /// Opcions:
  ///    potencia    Llindar de decisió per la potència.
  ///    r1norm      Llindar de correlació per la decisió sonor/sord. 
  ///    rmaxnorm    Llindar de correlació al max secundari per la decisió. 
  ///    zcr         Llindar de ZCR per la decisió sonor/sord.
  ///    window      Tipus de finestra (0=RECT, 1=HAMMING).
  ///    minf0       Freqüència mínima de pitch (Hz).
  ///    maxf0       Freqüència màxima de pitch (Hz).
  ///    median      Longitud del filtre de mediana (0=sense filtre).
  ///    clip        Llindar del central clipping (0=sense clipping).
  ///    method      Metode: 0=autocorr, 1=amdf, 2=cepstrum, 3=combinat.
  
    std::map<std::string, docopt::value> args = docopt::docopt(USAGE,
        {argv + 1, argv + argc},	// array of arguments, without the program name
        true,    // show help if requested
        "2.0");  // version string

	std::string input_wav = args["<input-wav>"].asString();
	std::string output_txt = args["<output-txt>"].asString();
  float llindar_pot      = std::stof(args["--potencia"].asString());
  float llindar_r1norm   = std::stof(args["--r1norm"].asString());
  float llindar_rmaxnorm = std::stof(args["--rmaxnorm"].asString());
  float llindar_zcr      = std::stof(args["--zcr"].asString());
  float min_f0           = std::stof(args["--minf0"].asString());
  float max_f0           = std::stof(args["--maxf0"].asString());
  int   window_type      = std::stoi(args["--window"].asString());
  int   median_len       = std::stoi(args["--median"].asString());
  float clip_level       = std::stof(args["--clip"].asString());
  int   method           = std::stoi(args["--method"].asString());

  // Read input sound file
  unsigned int rate;
  vector<float> x;
  if (readwav_mono(input_wav, rate, x) != 0) {
    cerr << "Error reading input file " << input_wav << " (" << strerror(errno) << ")\n";
    return -2;
  }

  int n_len = rate * FRAME_LEN;
  int n_shift = rate * FRAME_SHIFT;

  // Define analyzer
  PitchAnalyzer::Window w = (window_type == 1) ? PitchAnalyzer::HAMMING : PitchAnalyzer::RECT;

  PitchAnalyzer analyzer(n_len, rate, w, min_f0, max_f0,
                        llindar_pot, llindar_r1norm, llindar_rmaxnorm,
                        llindar_zcr, method);
  /// \TODO Preprocess the input signal in order to ease pitch estimation. For instance,
  /// central-clipping or low pass filtering may be used.

  /// \DONE Preprocessat: central clipping: 
  /// Hem implementat el central clipping amb el llindar indicat a la variable clip_level.
  /// Hem fet proves amb diferents tipus de central clipping de 0.0 a 0.3 i hem comprovat que
  /// el resultat més òptim segons l'optimize.sh és el de 0.3. 
  //Center clipping
    if (clip_level > 0.0F) {
      float max_val = 0.0F;
      for (unsigned int i = 0; i < x.size(); ++i) {
        if (fabs(x[i]) > max_val)
          max_val = fabs(x[i]);
      }
      float clip_level = clip_level * max_val;
      for (unsigned int i = 0; i < x.size(); ++i) {
        float xi = x[i];
        x[i] = (xi >= 0 ? 1.0F : -1.0F) * max(0.0F, fabs(xi) - clip_level);
      }
    }
/*
  if (clip_level > 0.0F) {
      float max_val = *max_element(x.begin(), x.end());
      float thr = clip_level * max_val;
      for (auto &sample : x) {
          if (sample > thr)       sample -= thr;
          else if (sample < -thr) sample += thr;
          else                    sample = 0.0F;
      }
  }*/
  // Iterate for each frame and save values in f0 vector
  vector<float>::iterator iX;
  vector<float> f0;
  for (iX = x.begin(); iX + n_len < x.end(); iX = iX + n_shift) {
    float f = analyzer(iX, iX + n_len);
    f0.push_back(f);
  }

  /// \TODO
  /// Postprocess the estimation in order to supress errors. For instance, a median filter
  /// or time-warping may be used.

  /// \DONE Postprocessat: filtre de mediana
  /// Hem aplicat el filtre de mediana amb la longitud amb indicada a la variable.
  /// Hem fet proves amb diferents tipus de mediana (amb longituds 3, 5, 15, 25) 
  /// i hem comprovat que, encara que teòricament hauria de ser més precís, el resultat
  /// no millora significativament, ans el contrari.
if (median_len > 1) {
    vector<float> f0_filtered = f0;
    int half = median_len / 2;
    for (size_t i = half; i + half < f0.size(); ++i) {
        vector<float> win(f0.begin() + i - half, f0.begin() + i + half + 1);
        sort(win.begin(), win.end());
        f0_filtered[i] = win[median_len / 2];
    }
    f0 = f0_filtered;
}
  // Write f0 contour into the output file
  ofstream os(output_txt);
  if (!os.good()) {
    cerr << "Error reading output file " << output_txt << " (" << strerror(errno) << ")\n";
    return -3;
  }

  os << 0 << '\n'; //pitch at t=0
  for (iX = f0.begin(); iX != f0.end(); ++iX) 
    os << *iX << '\n';
  os << 0 << '\n';//pitch at t=Dur

  return 0;
}
