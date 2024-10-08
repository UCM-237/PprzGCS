#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "coordenadaswindow.h"

#include <QPushButton>
#include <QMenu>
#include <QAction>
#include <QPoint>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Conectar botones a sus respectivos slots
    connect(ui->button1, &QPushButton::clicked, this, &MainWindow::on_button1_clicked);
    connect(ui->button2, &QPushButton::clicked, this, &MainWindow::on_pushButton_2_clicked);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_button1_clicked()
{
    // Crear el menú contextual
    QMenu contextMenu(tr("Menú contextual"), this);

    // Crear las acciones para el menú
    QAction action1("Con mapa", this);
    QAction action2("Sin mapa", this);

    // Conectar las acciones a los slots si es necesario
    connect(&action1, &QAction::triggered, this, [this]() {
        EstrategiaSeleccionada = "Con Mapa";
        ui->button1->setText(EstrategiaSeleccionada);
        qDebug() << "Estrategia seleccionada: " << EstrategiaSeleccionada;
    });

    connect(&action2, &QAction::triggered, this, [this]() {
        EstrategiaSeleccionada = "Sin Mapa";
        ui->button1->setText(EstrategiaSeleccionada);
        qDebug() << "Estrategia seleccionada: " << EstrategiaSeleccionada;
    });

    // Añadir las acciones al menú
    contextMenu.addAction(&action1);
    contextMenu.addAction(&action2);

    // Mostrar el menú en la posición del botón
    QPoint pos = ui->button1->mapToGlobal(QPoint(ui->button1->width()/2, ui->button1->height()/2));  // Centrar el menú en el botón
    contextMenu.exec(pos);
}

void MainWindow::on_pushButton_2_clicked()
{
    // Abrir el archivo en modo escritura
    QFile file("datos.txt");

    // Leer el texto ingresado en los QLineEdit
    NumeroSegmentos = ui->label1->text();
    NumeroPuntosControl = ui->label2->text();

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);  // Crear flujo de texto para escribir en el archivo

        //out <<"Estrategia seleccionada, Número de segmentos, Número de puntos de control\n";

        // Escribir los datos en el archivo
        out << "Estrategia seleccionada: " << EstrategiaSeleccionada << "\n";
        out << "Número de segmentos: " << NumeroSegmentos << "\n";
        out << "Número de puntos de control: " << NumeroPuntosControl << "\n";

        file.close();  // Cerrar el archivo
        close();
        // Mostrar mensaje de confirmación
        QMessageBox::information(this, "Guardar", "Datos guardados correctamente en datos.txt");

        // Obtener el número de coordenadas de label2
        bool ok;
        int numCoordenadas = NumeroPuntosControl.toInt(&ok);

        if (ok && numCoordenadas > 0) {
            qDebug() << "Abriendo la ventana de coordenadas";

            // Crear y mostrar la nueva ventana
            CoordenadasWindow *coordenadawindow = new CoordenadasWindow(numCoordenadas, nullptr);
            coordenadawindow->setWindowFlags(Qt::Window);
            coordenadawindow->move(100, 100);  // Mueve la ventana a la posición (100, 100)
            coordenadawindow->setWindowTitle("Ingresar Coordenadas");
            coordenadawindow->show();
        } else {
            // Mostrar un mensaje de error si el número no es válido
            QMessageBox::warning(this, "Error", "Por favor, ingrese un número válido en label2.");
        }
    } else {
        // Mostrar mensaje de error si el archivo no se puede abrir
        QMessageBox::warning(this, "Error", "No se pudo abrir el archivo para guardar los datos.");
    }
}

void MainWindow::on_NSeg_editingFinished()
{
    // Leer los valores ingresados en los QLineEdit
    NumeroSegmentos = ui->label1->text();
    qDebug() << "Valor en label1:" << NumeroSegmentos;
}

void MainWindow::on_NPntsContrl_editingFinished()
{
    // Leer los valores ingresados en los QLineEdit
    NumeroPuntosControl = ui->label2->text();
    qDebug() << "Valor en label2:" << NumeroPuntosControl;
}
