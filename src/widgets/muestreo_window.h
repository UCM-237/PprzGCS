#ifndef MUESTREO_WINDOW_H
#define MUESTREO_WINDOW_H

#include <QWidget>
#include <QStringListModel>

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
    void on_button_explorer_referencia_clicked();
    void on_button_explorer_flight_plan_clicked();
    void on_button_explorer_medidas_clicked();
    void on_button_open_flight_plan_clicked();
    void on_button_ver_datos_mision_clicked();
    void extraccion_datos(bool mostrarDespues, const QString &jsonFilePath = nullptr, const QString &csvFilePath = nullptr, const QString &jsonFilePath_sonda = nullptr, const QString &csvFilePath_sonda = nullptr);
    void mostrar_datos_mision();
    void guardarVentanaYCsvEnJson(const QString &jsonFilePath, const QString &csvFilePath);
    void guardarVentanaYCsvEnJson_sonda(const QString &jsonFilePath, const QString &csvFilePath);

private:
    Ui::muestreo_window *ui;

    //Definimos las variables donde se guardan los label
    QString homeDir;
    QString Responsable;
    QString Lugar;
    QString Referencia;
    QString Mision;
    QString valor_ficocianina;
    QString std_ficocianina;
    QString N_ficocianina;
    QString valor_clorofila;
    QString std_clorofila;
    QString N_clorofila;
    QString archivo_calibracion; 
    QString archivo_medidas;
    QString periodo_medidas;
    QString incidencias;
    QString flight_plan;

    QStringListModel *model;

    void loadFilesFromDirectory(const QString &path, QStringListModel *model, const QStringList &filters = QStringList());
};

#endif // MUESTREO_WINDOW_H
