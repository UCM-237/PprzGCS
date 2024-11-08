#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QPushButton>
#include <QMenu>
#include <QAction>
#include <QPoint>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QDebug>
#include <QProcess>  // Para ejecutar el script Python
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Conectar botones a sus respectivos slots
    connect(ui->button_estrategia, &QPushButton::clicked, this, &MainWindow::on_button_estrategia_clicked);
    connect(ui->button_optimizacion, &QPushButton::clicked, this, &MainWindow::on_button_optimizacion_clicked);
    connect(ui->button_compilacion, &QPushButton::clicked, this, &MainWindow::on_button_compilacion_clicked);
    connect(ui->button_editor, &QPushButton::clicked, this, &MainWindow::on_button_editor_clicked);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_button_estrategia_clicked()
{
    // Crear el menú contextual
    QMenu contextMenu(tr("Menú contextual"), this);

    // Crear las acciones para el menú
    QAction action1("Con Mapa", this);
    QAction action2("Sin mapa", this);

    // Conectar las acciones a los slots si es necesario
    connect(&action1, &QAction::triggered, this, [this]() {
        EstrategiaSeleccionada = "Con Mapa";
        ui->button_estrategia->setText(EstrategiaSeleccionada);
        qDebug() << "Estrategia seleccionada: " << EstrategiaSeleccionada;
    });

    connect(&action2, &QAction::triggered, this, [this]() {
        EstrategiaSeleccionada = "Sin Mapa";
        ui->button_estrategia->setText(EstrategiaSeleccionada);
        qDebug() << "Estrategia seleccionada: " << EstrategiaSeleccionada;
    });

    // Añadir las acciones al menú
    contextMenu.addAction(&action1);
    contextMenu.addAction(&action2);

    // Mostrar el menú en la posición del botón
    QPoint pos = ui->button_estrategia->mapToGlobal(QPoint(ui->button_estrategia->width()/2, ui->button_estrategia->height()/2));  // Centrar el menú en el botón
    contextMenu.exec(pos);
}

void MainWindow::on_button_optimizacion_clicked()
{
    disconnect(ui->button_optimizacion, &QPushButton::clicked, this, &MainWindow::on_button_optimizacion_clicked);

    QFile file("~/PprzGCS/datos.txt");

    Ruta_mapa = ui->label_mapa->text();
    Ruta_controlador = ui->label_controlador->text();
    Puntos_paso = ui->label_Puntos_paso->text();

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "Estrategia seleccionada: " << EstrategiaSeleccionada << "\n";
        out << "Ruta mapa:" << Ruta_mapa << "\n";
        out << "Ruta controlador:" << Ruta_controlador << "\n";
        out << "Numero de puntos de paso:" << Puntos_paso << "\n";
        file.close();

        QMessageBox::information(this, "Guardar", "Datos guardados correctamente en datos.txt");

        QProcess *process = new QProcess(this);
        QString scriptPath = "~/PprzGCS/Python_sw/Código_QT_PYTHON_V1_2.py";
        process->start("python", QStringList() << scriptPath);

        if (!process->waitForStarted()) {
            qDebug() << "Error al iniciar el script Python:" << process->errorString();
        }

        process->waitForFinished();
        QString output = process->readAllStandardOutput();
        QString errorOutput = process->readAllStandardError();

        // Analizar la salida JSON del script Python
        QJsonDocument jsonResponse = QJsonDocument::fromJson(output.toUtf8());
        if (!jsonResponse.isNull() && jsonResponse.isObject()) {
            QJsonObject jsonObj = jsonResponse.object();
            QString status = jsonObj.value("status").toString();
            QString message = jsonObj.value("message").toString();

            if (status == "success") {
                QMessageBox::information(this, "Ejecución exitosa", "El script de Python se ejecutó correctamente.");
            } else {
                QMessageBox::warning(this, "Error en la ejecución", "El script de Python finalizó con errores:\n" + message);
                qDebug() << "Error en la ejecución del script Python. Mensaje:" << message;
            }
        } else {
            QMessageBox::warning(this, "Error de salida", "La salida del script de Python no es válida:\n" + output);
            qDebug() << "Salida no válida del script Python:" << output;
        }

        process->deleteLater();
    }
}


void MainWindow::on_button_compilacion_clicked()
{
    disconnect(ui->button_compilacion, &QPushButton::clicked, this, &MainWindow::on_button_compilacion_clicked);

    // Crear un proceso para ejecutar el script Python
    QProcess *process_compilacion = new QProcess(this);
    QString scriptPath_compilacion = "~/PprzGCS/Python_sw/compilacion_paparazzi.py";

    // Usa la ruta completa al ejecutable de Python
    process_compilacion->start("python", QStringList() << scriptPath_compilacion);

    if (!process_compilacion->waitForStarted()) {
        qDebug() << "Error al iniciar el script Python:" << process_compilacion->errorString();
        return;
    }

    // Esperar a que el proceso termine
    process_compilacion->waitForFinished();
    int exitCode = process_compilacion ->exitCode();
    QString output = process_compilacion ->readAllStandardOutput();
    QString errorOutput = process_compilacion ->readAllStandardError();  // Capturar errores

    // Mostrar la salida y los errores en la consola de depuración
    qDebug() << "Salida del script Python:" << output;
    qDebug() << "Error del script Python:" << errorOutput;

    // Mostrar mensaje de confirmación según el resultado del script Python
    if (exitCode == 0) {
        QMessageBox::information(this, "Ejecución exitosa", "El script de Python se ejecutó correctamente.");
    } else {
        QMessageBox::warning(this, "Error en la ejecución", "El script de Python finalizó con errores.");
        qDebug() << "Error en la ejecución del script Python. Código de salida:" << exitCode;
    }

    process_compilacion ->deleteLater(); // Eliminar el proceso después de ejecutarse
    this->close();
}

void MainWindow::on_button_editor_clicked()
{
    disconnect(ui->button_editor, &QPushButton::clicked, this, &MainWindow::on_button_editor_clicked);

    // Crear un proceso para ejecutar el script Python
    QProcess *process_editor = new QProcess(this);
    QString scriptPath_editor= "~/PprzGCS/Python_sw/open_flight_plan_editor.py";

    // Usa la ruta completa al ejecutable de Python
    process_editor->start("python", QStringList() << scriptPath_editor);

    if (!process_editor->waitForStarted()) {
        qDebug() << "Error al iniciar el script Python:" << process_editor->errorString();
        return;
    }

    // Esperar a que el proceso termine
    process_editor->waitForFinished();
    int exitCode = process_editor->exitCode();
    QString output = process_editor->readAllStandardOutput();
    QString errorOutput = process_editor->readAllStandardError();  // Capturar errores

    // Mostrar la salida y los errores en la consola de depuración
    qDebug() << "Salida del script Python:" << output;
    qDebug() << "Error del script Python:" << errorOutput;
    process_editor ->deleteLater(); // Eliminar el proceso después de ejecutarse
    this->close();
}





/*
void MainWindow::on_ruta_mapa_editingFinished()
{
    // Leer los valores ingresados en los QLineEdit
    Ruta_mapa = ui->label_mapa->text();
    qDebug() << "Valor en label1:" << Ruta_mapa;
}

void MainWindow::on_ruta_controlador_editingFinished()
{
    // Leer los valores ingresados en los QLineEdit
    Ruta_controlador = ui->label_controlador->text();
    qDebug() << "Valor en label2:" << Ruta_controlador;
}

void MainWindow::on_Puntos_paso_editingFinished()
{
    // Leer los valores ingresados en los QLineEdit
    Puntos_paso = ui->label_Puntos_paso->text();
    qDebug() << "Valor en label3:" << Puntos_paso;
}
*/
