#include <stdio.h>
#include <stdlib.h>
#include <cfloat>
#include <limits>
#include <cmath>
#include "gvf_traj_lines_param.h"

// Color of trajectories
#define COLOR_YELLOW 0
#define COLOR_GREEN  1
#define COLOR_RED    2
#define COLOR_BLUE   3

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
 */
#define FACTOR       1.0 // TODO: Not used

// Path to files where data is saved. Choose the path you like.
const char x_val[]  = "var/conf/gvf_parametric_curve_x_values.data";
const char y_val[]  = "var/conf/gvf_parametric_curve_y_values.data";
const char ks_val[] = "var/conf/gvf_parametric_curve_ctrl_values.data";

GVF_traj_lines_param::GVF_traj_lines_param(QString id, QList<float> param, QList<float> _phi,
                                           float wb, QVector<int> *gvf_settings) :
  GVF_trajectory(id, gvf_settings)
{
  set_param(param, _phi, wb);
  generate_trajectory();
}

// 2D bezier trajectory (parametric representation)
void GVF_traj_lines_param::genTraj()
{

  QList<QPointF> points;
  QList<QPointF> points_conv;
  // Normal: 0; Mollified: 1
  float max_t = n_seg * FACTOR;
  float num_pts = n_seg  * FACTOR * 50;
  float dt = max_t / num_pts;

  for (float t = 0; t < max_t; t += dt)
  {
    points.append(eval_traj(t, 0));
    points_conv.append(eval_traj(t, 1));
  }
  createTrajItem(points, COLOR_BLUE, 0);
  createTrajItem(points_conv, COLOR_YELLOW, 1);
}

// 2D bezier GVF
void GVF_traj_lines_param::genVField()
{
  FILE *file_x; FILE *file_y; FILE *file_ks;

  // Only draw the file when data is available
  int cont = 0;
  if ((file_x = fopen(x_val, "r")) != NULL) {
    cont++;
    fclose(file_x);
  }
  if ((file_y = fopen(y_val, "r")) != NULL) {
    cont++;
    fclose(file_y);
  }
  if ((file_ks = fopen(ks_val, "r")) != NULL) {
    cont++;
    fclose(file_ks);
  }

  if (cont == 3) {
    QList<QPointF> vxy_mesh;
    float xmin, xmax, ymin, ymax;
    xmin = xx[0];
    xmax = xx[0];
    ymin = yy[0];
    ymax = yy[0];
    int n_pts = n_seg + 1;
    for (int k = 0; k < n_pts; k++)
    {
      xmax = (xx[k] > xmax) ? xx[k] : xmax;
      xmin = (xx[k] < xmin) ? xx[k] : xmin;
      ymax = (yy[k] > ymax) ? yy[k] : ymax;
      ymin = (yy[k] < ymin) ? yy[k] : ymin;
    }

    // TODO: Replace magic numbers
    float bound_area = 20 * (xmax - xmin) * (ymax - ymin);
    emit DispatcherUi::get()->gvf_defaultFieldSettings(ac_id, round(bound_area), 30, 30);
    xy_mesh = meshGrid();

    const int molli_flag = 1;

    foreach (const QPointF &point, xy_mesh)
    {
      // Evaluate only the smoothed trajectory
      float phix = point.x() - eval_traj(w, molli_flag).x(); // Normal component
      float phiy = point.y() - eval_traj(w, molli_flag).y(); // Normal Component
      float sigx = beta * eval_traj_der(w).x();              // Tangential Component
      float sigy = beta * eval_traj_der(w).y();              // Tangential Component
      float vx = sigx - kx * phix;
      float vy = sigy - ky * phiy;
      float norm = sqrt(pow(vx, 2) + pow(vy, 2));
      norm = (norm > 0) ? norm : 1; // Avoid division by zero
      vxy_mesh.append(QPointF(vx / norm, vy / norm));
    }
    createVFieldItem(xy_mesh, vxy_mesh);
  } else {
    fprintf(stderr, "Field cannot be created yet, waiting for complete data...\n");

  }
}

/////////////// PRIVATE FUNCTIONS ///////////////
void GVF_traj_lines_param::set_param(QList<float> param, QList<float> _phi, float wb)
{

  FILE *file_x; FILE *file_y; FILE *file_ks;
  int k; int n_pts;

  // Write:
  if (param[0] < 0)
  {
    n_seg = -(int)param[0];
    n_pts = n_seg + 1;
    if ((file_x = fopen(x_val, "w+")) != NULL)
    {
      // +1 Because it includes the param[0]
      for (k = 0; k < n_pts + 1; k++)
      {
        fprintf(file_x, "%f ", param[k]);
      }
      fclose(file_x);
    }
  }
  else if (param[0] > 0)
  {
    n_seg = (int)param[0];
    n_pts = n_seg + 1;
    if ((file_y = fopen(y_val, "w+")) != NULL)
    {
      // +1 Because it includes the param[0]
      for (k = 0; k < n_pts + 1; k++)
      {
        fprintf(file_y, "%f ", param[k]);
      }
      fclose(file_y);
    }
  }
  else
  {
    if ((file_ks = fopen(ks_val, "w+")) != NULL)
    {
      fprintf(file_ks, "%f %f %f %f %f", param[1], param[2], param[3], param[4], param[5]);
      fclose(file_ks);
    }
  }

  float N_SEG;

  // Read:
  if ((file_x = fopen(x_val, "r")) != NULL)
  {
    fscanf(file_x, "%f ", &N_SEG);
    n_seg = -(int)N_SEG;
    n_pts = n_seg + 1;
    for (k = 0; k < n_pts; k++)
    {
      fscanf(file_x, "%f ", &xx[k]);
    }
    fclose(file_x);
  }
  if ((file_y = fopen(y_val, "r")) != NULL)
  {
    fscanf(file_y, "%f ", &N_SEG);
    n_seg = (int)N_SEG;
    n_pts = n_seg + 1;
    for (k = 0; k < n_pts; k++)
    {
      fscanf(file_y, "%f ", &yy[k]);
    }
    fclose(file_y);
  }
  if ((file_ks = fopen(ks_val, "r")) != NULL)
  {
    fscanf(file_ks, "%f", &kx);
    fscanf(file_ks, "%f", &ky);
    fscanf(file_ks, "%f", &epsilon_x);
    fscanf(file_ks, "%f", &epsilon_y);
    fscanf(file_ks, "%f", &beta);
    fclose(file_ks);
  }

  phi = QPointF(_phi[0], _phi[1]);   //TODO: Display error in GVF viewer??
  w = (beta > 0) ? wb / beta : wb;   // gvf_parametric_w = wb/beta (wb = w*beta)
}

// Just in one dimension
float GVF_traj_lines_param::function_one_dimension(float *points, float lambda)
{
  float integer_part_float;
  int   integer_part;
  float fractional_part;
  float lambda_factor = lambda / FACTOR;
  fractional_part = modff(lambda_factor, &integer_part_float);
  integer_part = (int)(integer_part_float);
  // If the convolution parameter falls below the valid range,repeat the first segment.
  if(lambda_factor <= 1)
  {
    return (1 - lambda_factor) * points[0] +  lambda_factor * points[1];
  }
  else if(integer_part < n_seg)
  {
    return (1 - fractional_part) * points[integer_part] + fractional_part * points[integer_part+1];
  }
  else
  {
    // If the convolution parameter is above the valid range,repeat the last segment.
    return (1 - fractional_part) * points[n_seg - 1] + fractional_part * points[n_seg];
  }
}

float GVF_traj_lines_param::mollifier_one_dimension(float x, float epsilon)
{
  // TODO: Replace magic number
  float integration_constant = 0.44399;
  float y = x / epsilon;

  if(fabsf(y) < 1)
  {
    // Avoid divisions by zero. TODO Remove magic number
    if(fabsf(1-powf(y,2)) <= FLT_EPSILON)
    {
      return 0.0;
    }
    return 1 / (integration_constant * epsilon) * expf(-1 / (1-powf(y,2)));
  }
  return 0.0;
}

float GVF_traj_lines_param::mollifier_one_dimension_derivative(float x, float epsilon)
{
  // Avoid divisions by zero. TODO Remove magic number
  if(fabsf(powf(epsilon,2) - powf(x,2)) <= FLT_EPSILON)
  {
    return 0;
  }
  float fun_dot_f = -2 * powf(epsilon, 2) * x / (powf(epsilon,2) - powf(x,2));
  return mollifier_one_dimension(x, epsilon) * fun_dot_f;
}

// Convolution in one dimension
float GVF_traj_lines_param::convolution_one_dimension(float lambda, float *points,
                                                      int n_segments, float epsilon,
                                                      int order)
{
  // TODO: Replace magic number
  int n_points_of_integration = 100;

  /*
   * NOTE: The subtraction of epsilon is due to the definition of the function.
   *
   * Suppose the function is defined as f : [0, n] → ℝ. In the convolution,
   * the integration limits for y should satisfy:
   *
   *     y ∈ [max(-n + x, -ε), min(x, ε)]
   *
   * This ensures the integration remains within the defined domain.
   *
   * Due to the boundary conditions specified in gvf_parametric_bare_2d_lines_function,
   * we can integrate over the support of the mollifier, since for any integration
   * value outside the support of the mollifier the first and last segments are
   * repeated, thus convolving with the first and lasts segments.
   * */
  float lower_integration_value = -epsilon;
  float upper_integration_value = epsilon;

  float step_of_integration = (upper_integration_value - lower_integration_value) / n_points_of_integration;

  float convolution_at_lambda = 0;
  float step = 0;

  for(int k_iter = 0; k_iter < n_points_of_integration; k_iter++)
  {
    step = k_iter * step_of_integration + step_of_integration;

    // TODO: Replace with a more efficient approach so the "if" is not evaluated
    // in each iteration
    if(order == 0)
    {
      convolution_at_lambda += mollifier_one_dimension(lower_integration_value +
      step, epsilon) * function_one_dimension(points, lambda -
      (lower_integration_value + step)) * step_of_integration;
    }
    else if(order == 1)
    {
      convolution_at_lambda +=
      mollifier_one_dimension_derivative(lower_integration_value + step,
      epsilon) * function_one_dimension(points, lambda - (lower_integration_value
      + step)) * step_of_integration;
    }
  }

  return convolution_at_lambda;
}

QPointF GVF_traj_lines_param::eval_traj(float lambda, int which_traj)
{
  // Just in case w from telemetry is not between bounds
  float fx, fy;
  if (lambda < 0.0)
  {
    lambda = 0.0;
  }
  else if (lambda >= (float)(n_seg * FACTOR))
  {
    lambda = (float)(n_seg * FACTOR);
  }

  if(which_traj == 0)
  {
    fx = function_one_dimension(xx, lambda);
    fy = function_one_dimension(yy, lambda);
  }
  else if(which_traj == 1)
  {
    fx = convolution_one_dimension(lambda, xx, n_seg, epsilon_x, 0);
    fy = convolution_one_dimension(lambda, yy, n_seg, epsilon_y, 0);
  }

  return QPointF(fx, fy);
}

QPointF GVF_traj_lines_param::eval_traj_der(float lambda){
  // Just in case w from telemetry is not between bounds
  if (lambda < 0.0)
  {
    lambda = 0.0;
  }
  else if (lambda >= (float)((n_seg + 1) * FACTOR))
  {
    lambda = (float)(n_seg * FACTOR);
  }

  float fx = convolution_one_dimension(lambda, xx, n_seg, epsilon_x, 1);
  float fy = convolution_one_dimension(lambda, yy, n_seg, epsilon_y, 1);

  return QPointF(fx, fy);
}
