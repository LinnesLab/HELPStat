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
#include "Arduino.h"

// Internal variable used to monitor status of Levenberg-Marquardt algorithm. No related functionality (yet)
// -2: NotStarted
// -1: Running
//  0: ImproperInputParameters
//  1: RelativeReductionTooSmall
//  2: RelativeErrorTooSmall
//  3: RelativeErrorAndReductionTooSmall
//  4: CosinusTooSmall
//  5: TooManyFunctionEvaluation
//  6: FtolTooSmall
//  7: XtolTooSmall
//  8: GtolTooSmall
//  9: UserAsked
int status;

//=================================================================================================================
// LMFunctorRct
// Description: struct used for performing Levenberg-Marquardt algorithm to determine Rct.
// 
// int operator() - Calculates the error using current model parameters. For Rct, the error was estimated using:
//    error = Zim^2 - [(Rct/2)^2  -  (Zre - Rs - Rct/2)^2]
// int df()       - Calculates the Jacobian matrix of the errors. Used to create better estimates for parameters.
//=================================================================================================================
struct LMFunctorRct {
	// 'm' pairs of (x, f(x))
	Eigen::MatrixXf measuredValues;

	// Compute 'm' errors, one for each data point, for the given parameter values in 'x'
	int operator()(const Eigen::VectorXf &x, Eigen::VectorXf &fvec) const
	{
		// 'x' has dimensions n x 1
		// It contains the current estimates for the parameters.

		// 'fvec' has dimensions m x 1
		// It will contain the error for each data point.

		float Rct = x(0);
		float Rs  = x(1);

		for (int i = 0; i < values(); i++) {
			float xValue = measuredValues(i, 0);
			float yValue = measuredValues(i, 1);

      fvec(i) = yValue*yValue - (0.25*Rct*Rct - (xValue - Rs - 0.5*Rct)*(xValue - Rs - 0.5*Rct)); // So far the best for Rct, but overestimates Rs.
    }
		return 0;
	}

	// Compute the jacobian of the errors
	int df(const Eigen::VectorXf &x, Eigen::MatrixXf &fjac) const
	{
		// 'x' has dimensions n x 1
		// It contains the current estimates for the parameters.

		// 'fjac' has dimensions m x n
		// It will contain the jacobian of the errors, calculated numerically in this case.

		float epsilon = 1;
		// epsilon = 1e-5f;

		for (int i = 0; i < x.size(); i++) {
			Eigen::VectorXf xPlus(x);
			xPlus(i) += epsilon;
			Eigen::VectorXf xMinus(x);
			xMinus(i) -= epsilon;

			Eigen::VectorXf fvecPlus(values());
			operator()(xPlus, fvecPlus);

			Eigen::VectorXf fvecMinus(values());
			operator()(xMinus, fvecMinus);

			Eigen::VectorXf fvecDiff(values());
			fvecDiff = (fvecPlus - fvecMinus) / (2.0f * epsilon);

			fjac.block(0, i, values(), 1) = fvecDiff;
		}

		return 0;
	}

	// Number of data points, i.e. values.
	int m;

	// Returns 'm', the number of values.
	int values() const { return m; }

	// The number of parameters, i.e. inputs.
	int n;

	// Returns 'n', the number of inputs.
	int inputs() const { return n; }

};

//=================================================================================================================
// LMFunctorRs
// Description: struct used for performing Levenberg-Marquardt algorithm to determine Rs.
// 
// int operator() - Calculates the error using current model parameters. For Rs, the error was estimated using:
//    error = Zim - sqrt{abs[(Rct/2)^2  -  (Zre - Rs - Rct/2)^2]}
// int df()       - Calculates the Jacobian matrix of the errors. Used to create better estimates for parameters.
//=================================================================================================================
struct LMFunctorRs {
	// 'm' pairs of (x, f(x))
	Eigen::MatrixXf measuredValues;

	// Compute 'm' errors, one for each data point, for the given parameter values in 'x'
	int operator()(const Eigen::VectorXf &x, Eigen::VectorXf &fvec) const
	{
		// 'x' has dimensions n x 1
		// It contains the current estimates for the parameters.

		// 'fvec' has dimensions m x 1
		// It will contain the error for each data point.

		float Rct = x(0);
		float Rs  = x(1);

		for (int i = 0; i < values(); i++) {
			float xValue = measuredValues(i, 0);
			float yValue = measuredValues(i, 1);

      fvec(i) = yValue - sqrt(abs(0.25*Rct*Rct - (xValue - Rs - 0.5*Rct)*(xValue - Rs - 0.5*Rct)));
    }
		return 0;
	}

	// Compute the jacobian of the errors
	int df(const Eigen::VectorXf &x, Eigen::MatrixXf &fjac) const
	{
		// 'x' has dimensions n x 1
		// It contains the current estimates for the parameters.

		// 'fjac' has dimensions m x n
		// It will contain the jacobian of the errors, calculated numerically in this case.

		float epsilon = 1;
		// epsilon = 1e-5f;

		for (int i = 0; i < x.size(); i++) {
			Eigen::VectorXf xPlus(x);
			xPlus(i) += epsilon;
			Eigen::VectorXf xMinus(x);
			xMinus(i) -= epsilon;

			Eigen::VectorXf fvecPlus(values());
			operator()(xPlus, fvecPlus);

			Eigen::VectorXf fvecMinus(values());
			operator()(xMinus, fvecMinus);

			Eigen::VectorXf fvecDiff(values());
			fvecDiff = (fvecPlus - fvecMinus) / (2.0f * epsilon);

			fjac.block(0, i, values(), 1) = fvecDiff;
		}

		return 0;
	}

	// Number of data points, i.e. values.
	int m;

	// Returns 'm', the number of values.
	int values() const { return m; }

	// The number of parameters, i.e. inputs.
	int n;

	// Returns 'n', the number of inputs.
	int inputs() const { return n; }

};

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
float calculate_Rct(float rct_estimate, float rs_estimate, std::vector<float> r_data, std::vector<float> i_data) {
	Eigen::VectorXf param_rct(2); // Estimates
	param_rct(0) = rct_estimate;
	param_rct(1) = rs_estimate;
  
	LMFunctorRct functor_rct;
  
	int m = r_data.size();
  
	Eigen::MatrixXf measuredValues(m, 2);
	for(int i = 0; i < m; i++) {
	  measuredValues(i,0) = r_data[i];
	  measuredValues(i,1) = i_data[i];
	}
  
	functor_rct.measuredValues = measuredValues;
	functor_rct.m = m;
	functor_rct.n = 2;
  
	Eigen::LevenbergMarquardt<LMFunctorRct, float> lm_rct(functor_rct);
	status = lm_rct.minimize(param_rct);
	
	return(param_rct(0));
  }

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
float calculate_Rs(float rct_estimate, float rs_estimate, std::vector<float> r_data, std::vector<float> i_data) {
	Eigen::VectorXf param_rs(2); // Estimates
	param_rs(0) = rct_estimate;
	param_rs(1) = rs_estimate;
  
	LMFunctorRs functor_rs;
  
	int m = r_data.size();
  
	Eigen::MatrixXf measuredValues(m, 2);
	for(int i = 0; i < m; i++) {
	  measuredValues(i,0) = r_data[i];
	  measuredValues(i,1) = i_data[i];
	}
  
	functor_rs.measuredValues = measuredValues;
	functor_rs.m = m;
	functor_rs.n = 2;
  
	Eigen::LevenbergMarquardt<LMFunctorRs, float> lm_rs(functor_rs);
	status = lm_rs.minimize(param_rs);
	
	return(param_rs(1));
  }