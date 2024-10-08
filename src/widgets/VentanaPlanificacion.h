#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include<QPushButton>
#include<QMenu>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_button1_clicked();

    void on_pushButton_2_clicked();

    void on_NSeg_editingFinished();

    void on_NPntsContrl_editingFinished();


private:
    Ui::MainWindow *ui;
    QString EstrategiaSeleccionada;
    QString NumeroSegmentos;
    QString NumeroPuntosControl;
};
#endif // MAINWINDOW_H
