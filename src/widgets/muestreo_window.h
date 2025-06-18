#ifndef MUESTREO_WINDOW_H
#define MUESTREO_WINDOW_H

#include <QWidget>

//Declara la clase .ui
namespace Ui {
class muestreo_window;  // debe coincidir con el nombre del formulario en .ui
}

//Declaro mi clase muestreo_window que hereda de QWidget
class muestreo_window : public QWidget {
    Q_OBJECT

public:
    //Constructor de mi clase. Es lo que se ejecuta cuando haces muestreo_window* w = new muestreo_window();
    explicit muestreo_window(QWidget *parent = nullptr);
    //Destructor de la clase. Se llama automáticamente cuando el objeto se destruye para liberar memoria.
    ~muestreo_window();

private slots:
    void on_button_save_clicked();

private:
    Ui::muestreo_window *ui;

    //Definimos las variables donde se guardan los label
    QString homeDir;
    QString Responsable;
    QString Lugar;
    QString Referencia;
    QString h_inicio_ficocianina;
    QString h_fin_ficocianina;
    QString h_inicio_clorofila;
    QString h_fin_clorofila;
    QString archivo_calibracion; 
    QString archivo_medidas;
    QString periodo_medidas;
    QString incidencias;
};

#endif // MUESTREO_WINDOW_H
