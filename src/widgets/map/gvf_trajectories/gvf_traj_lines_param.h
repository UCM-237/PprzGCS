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
  explicit GVF_traj_lines_param(QString id, QList<float> param, QList<float> _phi,
                                float wb, QVector<int> *gvf_settings);

protected:
  virtual void genTraj() override;
  virtual void genVField() override;

private:
  void set_param(QList<float> param, QList<float> _phi, float wb); // GVF PARAMETRIC

  QPointF eval_traj(float lambda, int which_traj);

  QPointF eval_traj_der(float lambda);

  float convolution_one_dimension(float lambda, float *points,
                                  float epsilon, int order);

  float mollifier_one_dimension_derivative(float x, float epsilon);

  float mollifier_one_dimension(float x, float epsilon);

  float function_one_dimension(float *points, float lambda);

  // TODO: Replace magic number
  float xx[32];
  float yy[32];
  int n_seg;
  float w;
  float kx;
  float ky;
  float beta;
  float epsilon_x;
  float epsilon_y;
  uint8_t draw_normal_mollified;
  QPointF phi;
};

#endif // GVF_TRAJ_LINES_PARAM_H
