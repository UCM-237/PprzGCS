#include <stdio.h>
#include <stdlib.h>
#include <cfloat>
#include <limits>
#include <cmath>
#include "gvf_traj_lines_param.h"

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
  float max_t = n_seg * FACTOR;
  float num_pts = n_seg  * FACTOR * NUM_POINTS_CURVE_DRAWING;
  float dt = max_t / num_pts;

  for (float t = 0; t <= max_t; t += dt)
  {
    points.append(eval_traj(t, NORMAL_TRAJ));
    points_conv.append(eval_traj(t, MOLLIFIED_TRAJ));
  }
  createTrajItem(points, COLOR_BLUE, NORMAL_TRAJ);
  createTrajItem(points_conv, COLOR_YELLOW, MOLLIFIED_TRAJ);
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

    foreach (const QPointF &point, xy_mesh)
    {
      // Evaluate only the smoothed trajectory
      float phix = point.x() - eval_traj(w, MOLLIFIED_TRAJ).x(); // Normal component
      float phiy = point.y() - eval_traj(w, MOLLIFIED_TRAJ).y(); // Normal Component
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

  /*
   * IT IS NECESSARY TO EXTEND THE DOMAIN OF THE FUNCTION.
   *
   * This extension is required to ensure the convolution of the trajectory
   * is properly carried out. Each component of the trajectory
   * is given by a function f : [0, N_SEG] → ℝ. When convolving with a mollifier
   * whose support is [-ε, ε], the convolution requires evaluating f outside its
   * original domain, specifically, over the extended interval [-ε, N_SEG + ε].
   *
   * For this reason, the trajectory must be defined beyond its original bounds.
   * Values of the curve parameter below 0 and above N_SEG are handled by extending
   * the line segment.
   */

  if(lambda_factor <= 1)
  {
    // If the convolution parameter falls below the valid range,repeat the first segment.
    return (1 - fractional_part) * points[0] +  fractional_part * points[1];
  }
  else if(integer_part < n_seg)
  {
    return (1 - fractional_part) * points[integer_part] + fractional_part * points[integer_part + 1];
  }
  else
  {
    /* If the convolution parameter is above the valid range,repeat the last segment extending it
     * to arbitrary values of lambda */
    return (1 - (lambda_factor - n_seg + 1)) * points[n_seg - 1] + (lambda_factor - n_seg + 1) * points[n_seg];
  }
}

float GVF_traj_lines_param::mollifier_one_dimension(float x, float epsilon)
{
  float y = x / epsilon;

  if(fabsf(y) < 1)
  {
    // Avoid divisions by zero
    if(fabsf(1-powf(y,2)) <= FLT_EPSILON)
    {
      return 0.0;
    }
    return 1 / (INTEGRATION_CONSTANT * epsilon) * expf(-1 / (1-powf(y,2)));
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
                                                      float epsilon, int order)
{
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

  float step_of_integration = (upper_integration_value - lower_integration_value) / NUM_POINTS_OF_INTEGRATION;

  float convolution_at_lambda = 0;
  float step = 0;

  for(int k_iter = 0; k_iter < NUM_POINTS_OF_INTEGRATION; k_iter++)
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
  float fx, fy;
  // Just in case lambda is not between bounds
  if(lambda < 0.0)
  {
    lambda = 0.0;
  }
  else if(lambda >= n_seg)
  {
    lambda = n_seg;
  }

  if(which_traj == 0)
  {
    fx = function_one_dimension(xx, lambda);
    fy = function_one_dimension(yy, lambda);
  }
  else if(which_traj == 1)
  {
    fx = convolution_one_dimension(lambda, xx, epsilon_x, 0);
    fy = convolution_one_dimension(lambda, yy, epsilon_y, 0);
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

  float fx = convolution_one_dimension(lambda, xx, epsilon_x, 1);
  float fy = convolution_one_dimension(lambda, yy, epsilon_y, 1);

  return QPointF(fx, fy);
}
