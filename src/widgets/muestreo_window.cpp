#include "muestreo_window.h"
#include "ui_muestreo_window.h"

#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <QTextStream>

muestreo_window::muestreo_window(QWidget *parent) :
    QWidget(parent), //Llama al constructor padre QWidget(parent)
    ui(new Ui::muestreo_window) //Crea el puntero a la interfaz muestreo_window 
    {
    ui->setupUi(this);
    setWindowTitle("Muestreo");
    homeDir = QDir::homePath(); //Directorio home
    connect(ui->button_save, &QPushButton::clicked, this, &muestreo_window::on_button_save_clicked); //Botón para guardar
}

muestreo_window::~muestreo_window() {
    delete ui;
}

void muestreo_window::on_button_save_clicked()
{
    disconnect(ui->button_save, &QPushButton::clicked, this, &muestreo_window::on_button_save_clicked);
    QFile file( homeDir + "/PprzGCS/Planificacion/muestreo.txt");

    Responsable = ui->label_responsable->text();
    Lugar = ui->label_lugar->text();
    Referencia = ui->label_referencia->text();
    h_inicio_ficocianina = ui->label_h_inicio_ficocianina->text();
    h_fin_ficocianina = ui->label_h_fin_ficocianina->text();
    h_inicio_clorofila = ui->label_h_inicio_clorofila->text();
    h_fin_clorofila = ui->label_h_fin_clorofila->text();
    archivo_calibracion = ui->label_archivo_calibracion->text();
    archivo_medidas = ui->label_archivo_medidas->text();
    periodo_medidas = ui->label_periodo_medidas->text();
    incidencias = ui->label_incidencias->text();
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "Responsable: " << Responsable << "\n";
        out << "Lugar: " << Lugar << "\n";
        out << "Referencia:" << Referencia << "\n";
        out << "Hora inicio ficocianina: " << h_inicio_ficocianina << "\n";
        out << "Hora fin ficocianina: " << h_fin_ficocianina << "\n";
        out << "Hora inicio clorofila: " << h_inicio_clorofila << "\n";
        out << "Hora fin clorofila: " << h_fin_clorofila << "\n";
        out << "Ruta archivo calibracion: " << archivo_calibracion << "\n";
        out << "Ruta archivo medidas: " << archivo_medidas << "\n";
        out << "Incidencias: " << incidencias << "\n";

        file.close();

        QMessageBox::information(this, "Guardar", "Datos guardados correctamente en muestras.txt");
    }
}
