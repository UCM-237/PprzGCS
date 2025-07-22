#ifndef GVF_TRAJ_LINES_PARAM_H
#define GVF_TRAJ_LINES_PARAM_H

#include "gvf_trajectory.h"

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
                                  int n_segments, float epsilon,
                                  int order);

  float mollifier_one_dimension_derivative(float x, float epsilon);

  float mollifier_one_dimension(float x, float epsilon);

  float function_one_dimension(float *points, float lambda);

  // Maximum data size from gvf_parametric is 16 elements. But sometimes we need more
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
