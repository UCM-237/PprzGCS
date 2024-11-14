#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QMenu>
#include <QProcess>  // Para ejecutar scripts de Python

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
    void on_button_estrategia_clicked();
    void on_button_optimizacion_clicked();   
    void on_button_compilacion_clicked(); 
    void on_button_editor_clicked();  
    void on_button_datos_clicked(); 
    void VentanaSector();
    /*
    void on_ruta_mapa_editingFinished();
    void on_ruta_controlador_editingFinished();
    void on_Puntos_paso_editingFinished();
    */
private:
    Ui::MainWindow *ui;
    QString EstrategiaSeleccionada;
    QString Ruta_mapa;
    QString Ruta_controlador;
    QString Ruta_aircraft;
    QString Puntos_paso;
    QProcess *process;  // Para manejar el proceso del script Python

    void ejecutarScriptPython();  // Método para ejecutar el script y manejar su salida
};

#endif // MAINWINDOW_H

