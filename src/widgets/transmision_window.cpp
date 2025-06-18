#include "transmision_window.h"
#include "ui_transmision_window.h"

#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <QTextStream>

transmision_window::transmision_window(QWidget *parent) :
    QWidget(parent), //Llama al constructor padre QWidget(parent)
    ui(new Ui::transmision_window) //Crea el puntero a la interfaz muestreo_window 
    {
    ui->setupUi(this);
    setWindowTitle("Transmision");
    connect(ui->button_transmitir, &QPushButton::clicked, this, &transmision_window::on_button_send_clicked); //Botón para guardar
}

transmision_window::~transmision_window() {
    delete ui;
}

void transmision_window::on_button_send_clicked()
{
    //disconnect(ui->button_transmitir, &QPushButton::clicked, this, &transmision_window::on_button_send_clicked);
    
    QString input = ui->label_contrasena->text();
    
    if (input == "1234"){

        QMessageBox::information(this, "Contraseña", "Contraseña correcta");

    }
    else{

        QMessageBox::critical(this, "Contraseña", "Contraseña incorrecta");

    }
}
