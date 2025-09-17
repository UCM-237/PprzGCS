#ifndef GVF_TRAJ_LINES_PARAM_H
#define GVF_TRAJ_LINES_PARAM_H

#include "gvf_trajectory.h"

// Color of trajectories
#define COLOR_YELLOW 0
#define COLOR_GREEN  1
#define COLOR_RED    2
#define COLOR_BLUE   3

// Integration constant ensures that \int_{\R}\phi = 1, where \phi is the mollifier.
#define INTEGRATION_CONSTANT 0.44399

// Points of integration used to compute the convolution
#define NUM_POINTS_OF_INTEGRATION 100

// Points to draw the curve
#define NUM_POINTS_CURVE_DRAWING 100

// Normal trajectory
#define NORMAL_TRAJ 0

// Mollified trajectory
#define MOLLIFIED_TRAJ 1

// Max data points comming from telemetry
#define MAX_POINTS 32
/*
 * Reparametrization factor: adjusts the parameter range from [0, N_SEG]
 * to [0, FACTOR * N_SEG] to achieve finer control over the curve.
 *
 * Each segment between two points x, y ∈ \R^2 is linearly interpolated by:
 *
 *     λx + (1 - λ)y,   where λ ∈ [0, 1].
 *
 * The issue arises when ε = 0.5 — half of the final segment is excluded
 * due to the convolution's support. By increasing the parameter range
 * with a factor (e.g., FACTOR = 10), the effective resolution improves.
 * For example, with ε = 0.5 and FACTOR = 10, only 1/20 of the curve is cut.
 *
 * NOTE: This must be the same value as in Paparazzi.
 * TODO: Recieve this factor via telemetry so we do not need to change
 * it every time we change the original factor from paparazzi.
 */
#define FACTOR       1.0

// Path to files where data is saved. Choose the path you like.
const char x_val[]  = "var/conf/gvf_parametric_curve_x_values.data";
const char y_val[]  = "var/conf/gvf_parametric_curve_y_values.data";
const char ks_val[] = "var/conf/gvf_parametric_curve_ctrl_values.data";

class GVF_traj_lines_param : public GVF_trajectory
{
  Q_OBJECT
public:
  /* @ Constructor GVF_traj_lines_param
   * @brief Constructs the class
   * @param id
   * @param param : Messages from telemetry
   * @param phi : Phi from telemetry
   * @param wb: Parameter of trajectory from telemetry multiplied by beta
   * @param gvf_settings: GVF_settings
   */
  explicit GVF_traj_lines_param(QString id, QList<float> param, QList<float> _phi,
                                float wb, QVector<int> *gvf_settings);

protected:
  virtual void genTraj() override;
  virtual void genVField() override;

private:

  /* @function set_param
   * @brief Set the attributes of the class via the message from telemetry and saves
   * them in data files
   * @param param: Message from telemetry
   * @param _phi: Phi from telemetry
   * @param wb: Parameter of the curve multiplied by beta of the GVF
   * Returns: None
   */
  void set_param(QList<float> param, QList<float> _phi, float wb); // GVF PARAMETRIC

  /* @function eval_traj
   * @brief Function that evaluates the original or mollified trajectory
   * @param lambda: Point in which the trajectory must be evaluated
   * @param which_traj: Which trajectory must be evaluated: 0 for normal, 1 for mollified
   * Returns: Evaluated trajectory as QPointF
   */
  QPointF eval_traj(float lambda, int which_traj);

  /* @function eval_traj_der
   * @brief Function that evaluates the derivative of the mollified trajectory
   * @param lambda: Point in which the trajectory must be evaluated.
   * Returns: Evaluated derivative of trajectory as QPointF
   */
  QPointF eval_traj_der(float lambda);

  /* @function convolution_one_dimension
   * @brief Performs simple 1D convolution with mollifier or its derivative
   * @param lambda Parameter value
   * @param points Array of points to convolve
   * @param epsilon Smoothing parameter
   * @param order 0 for function, 1 for derivative
   * Returns: Convolution of the function with the mollifier at lambda
   */
  float convolution_one_dimension(float lambda, float *points,
                                  float epsilon, int order);

  /* @function mollifier_one_dimension
   * @brief Computes the mollifier function \phi for smoothing
   * @param x IN: Value in which the mollifier is evaluated
   * @param epsilon IN: Half of the length of the support of the mollifier [-\epsilon, \epsilon]
   * Returns: Evaluated mollifier \frac{1}{\epsilon}\phi(x/\epsilon)
   */
  float mollifier_one_dimension(float x, float epsilon);

  /* @function mollifier_one_dimension_derivative
   * @brief Computes the derivative of the mollifier function \phi for smoothing
   * @param x IN: Value in which the mollifier is evaluated
   * @param epsilon IN: Half of the length of the support of the mollifier [-\epsilon, \epsilon]
   * Returns: Evaluated derivative of the mollifier
   */
  float mollifier_one_dimension_derivative(float x, float epsilon);

  /* @function function_one_dimension
   * @brief Computes the desired function to be followed in one dimension
   * @param points IN: Arrays of points in one dimension
   * @param lambda IN: Value in which the function is evaluated
   * Returns: Evaluated function in one dimension
   */
  float function_one_dimension(float *points, float lambda);

  float xx[MAX_POINTS];   // X coordinate data points
  float yy[MAX_POINTS];   // Y coordinate data points
  int n_seg;              // Number of segments of the trajectory
  float w;                // Parameter of the trajectory
  float kx;               // Perpendicular X constant of the parametric GVF
  float ky;               // Perpendicular Y constant of the parametric GVF
  float beta;             // Beta constant of the parametric GVF
  float epsilon_x;        // Epsilon of the mollifier in X direction (half of length of its support)
  float epsilon_y;        // Epsilon of the mollifier in Y direction (half of length of its support)
  QPointF phi;            // Error to the path in both components
};

#endif // GVF_TRAJ_LINES_PARAM_H
