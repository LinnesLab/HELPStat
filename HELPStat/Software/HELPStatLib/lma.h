//=================================================================================================================
// This library is largely depedent on a modified version of Bolder Flight Systems' Eigen port for MPUs. This
// modified library adds nonlinear optimziation and Levenberg-Marquardt algorithm implementation:
//   Modified Eigen Port:               https://github.com/LinnesLab/Eigen-Port
// Documentation for the PC Eigen library and for Bolder Flight Systems' Eigen port can be found at:
//   Official Eigen Library Website:    https://eigen.tuxfamily.org/index.php?title=Main_Page
//   Bolder Flight Systems' Eigen Port: https://github.com/bolderflight/eigen
//  
// This library implements a Levenberg-Marquardt algorithm for calculating charge transfer and solution resistances
// for the equivalent Randel's cell for electrochemical impedence spectroscopy. The code for parameter optimization
// was adapted from the following examples:
//   https://github.com/SarvagyaVaish/Eigen-Levenberg-Marquardt-Optimization/tree/master?tab=readme-ov-file
//   https://github.com/daviddoria/Examples/blob/master/c%2B%2B/Eigen/LevenbergMarquardt/NumericalDerivative.cpp
//
//   The semicircle equation for the Nyquist plot is: Zim = sqrt[(Rct/2)^2  -  (Zre - Rs - Rct/2)^2]. However, note
//   that sqrt() returns NaN if its input is a negative number. To account for this and prevent errors due to
//   negative sqrt() inputs, I developed two methods for calculating error:
//   (1) error = Zim^2 - [(Rct/2)^2  -  (Zre - Rs - Rct/2)^2]
//   (2) error = Zim   - sqrt{abs[(Rct/2)^2  -  (Zre - Rs - Rct/2)^2]}
//
//   Upon testing, I found that this first method would calculate an Rct similar to Matlab's Levenberg-Marquardt
//   implementation at the expense of overestimating Rs by a factor of 10x. Meanwhile, the second method would
//   calculate an Rs similar to Matlab's Levenberg-Marquardt implementation at the expense of underestimating
//   Rct by roughly 10kOhm. To compromise, both equations were used; the first for calculating Rct and the second
//   for calculating Rs.
//=================================================================================================================
#include <vector>
#include "eigen.h"

//=================================================================================================================
// calculate_Rct
// Description: Given real and imaginary impedence data, this function calculates and returns the optimal Rct value
//              that fits the diameter of the Randel cell's Nyquist plot.
// Inputs:
// * float rct_estimate        - Initial estimate for Rct (charge transfer resistance) in ohms
// * float rs_estimate         - Initial estimate for Rs  (solution resistance) in ohms
// * std::vector<float> r_data - Vector of real impedence data in ohms
// * std::vector<float> i_data - Vector of imaginary impedence data in ohms
// Output:
// * float Rct                 - Optimized value for Rct (charge transfer resistance) in ohms
//=================================================================================================================
float calculate_Rct(float rct_estimate, float rs_estimate, std::vector<float> r_data, std::vector<float> i_data);

//=================================================================================================================
// calculate_Rs
// Description: Given real and imaginary impedence data, this function calculates and returns the optimal Rs value
//              that fits the x-intercept of the Randel cell's Nyquist plot.
// Inputs:
// * float rct_estimate        - Initial estimate for Rct (charge transfer resistance) in ohms
// * float rs_estimate         - Initial estimate for Rs  (series resistance) in ohms
// * std::vector<float> r_data - Vector of real impedence data in ohms
// * std::vector<float> i_data - Vector of imaginary impedence data in ohms
// Output:
// * float Rs                  - Optimized value for Rs (solution resistance) in ohms
//=================================================================================================================
float calculate_Rs(float rct_estimate, float rs_estimate, std::vector<float> r_data, std::vector<float> i_data);