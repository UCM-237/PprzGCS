#include "planificacionwindow.h"
#include "ui_planificacionwindow.h"
#include "sectors_window.h"
#include "AircraftManager.h"
#include "ui_sectors_window.h"
//#include "movewpopt.h"
#include "AircraftManager.h"
#include <QFileDialog>  // Incluir QFileDialog para explorar archivos
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
#include <QDir>
#include <stdint.h>
#include "gcs_utils.h"
#include <cstdint>
#include <QColor>
#include "waypointeditor.h"
#include "waypoint.h"
#include "waypoint_item.h"
#include "flightplan.h"

Q_DECLARE_METATYPE(pprzlink::Message)


PlanificacionWindow::PlanificacionWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::PlanificacionWindow)
{

    ui->setupUi(this);
    homeDir = QDir::homePath(); // Obtener la ruta del directorio home del usuario
    // Conectar botones a sus respectivos slots
    connect(ui->button_estrategia, &QPushButton::clicked, this, &PlanificacionWindow::on_button_estrategia_clicked);
    connect(ui->button_optimizacion, &QPushButton::clicked, this, &PlanificacionWindow::on_button_optimizacion_clicked);
    connect(ui->button_compilacion, &QPushButton::clicked, this, &PlanificacionWindow::on_button_compilacion_clicked);
    connect(ui->button_editor, &QPushButton::clicked, this, &PlanificacionWindow::on_button_editor_clicked);
    connect(ui->button_sectores, &QPushButton::clicked, this, &PlanificacionWindow::VentanaSector);
    connect(ui->button_datos, &QPushButton::clicked, this, &PlanificacionWindow::on_button_datos_clicked);
//    connect(ui->button_move_wp, &QPushButton::clicked, this, &PlanificacionWindow::on_button_move_wp_clicked);

    connect(ui->button_abrir_mapa, &QPushButton::clicked, this, &PlanificacionWindow::on_button_abrir_mapa_clicked);
    connect(ui->button_abrir_controlador, &QPushButton::clicked, this, &PlanificacionWindow::on_button_move_wp_clicked);

    connect(ui->button_move_wp, &QPushButton::clicked, this, &PlanificacionWindow::on_button_move_wp_clicked);
}

PlanificacionWindow::~PlanificacionWindow()
{
    delete ui;
}


void PlanificacionWindow::on_button_estrategia_clicked()
{
    // Crear el menú contextual
    QMenu contextMenu(tr("Menú contextual"), this);

    // Crear las acciones para el menú
    QAction action1("Con Mapa", this);
    QAction action2("Sin mapa", this);

    // Conectar las acciones a los slots si es necesario
    connect(&action1, &QAction::triggered, this, [this]() {
        EstrategiaSeleccionada = " Con Mapa";
        ui->button_estrategia->setText(EstrategiaSeleccionada);
        qDebug() << "Estrategia seleccionada:" << EstrategiaSeleccionada;
    });

    connect(&action2, &QAction::triggered, this, [this]() {
        EstrategiaSeleccionada = " Sin Mapa";
        ui->button_estrategia->setText(EstrategiaSeleccionada);
        qDebug() << "Estrategia seleccionada:" << EstrategiaSeleccionada;
    });

    // Añadir las acciones al menú
    contextMenu.addAction(&action1);
    contextMenu.addAction(&action2);

    // Mostrar el menú en la posición del botón
    QPoint pos = ui->button_estrategia->mapToGlobal(QPoint(ui->button_estrategia->width()/2, ui->button_estrategia->height()/2));  // Centrar el menú en el botón
    contextMenu.exec(pos);
}

void PlanificacionWindow::on_button_optimizacion_clicked()
{
    disconnect(ui->button_optimizacion, &QPushButton::clicked, this, &PlanificacionWindow::on_button_optimizacion_clicked);

    QFile file( homeDir + "/PprzGCS/Planificacion/datos.txt");

    Ruta_mapa = ui->label_mapa->text();
    Ruta_controlador = ui->label_controlador->text();
    Ruta_aircraft = ui->label_aircraft->text();
    Puntos_paso = ui->label_Puntos_paso->text();

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "Estrategia seleccionada: " << EstrategiaSeleccionada << "\n";
        out << "Ruta mapa:" << Ruta_mapa << "\n";
        out << "Ruta controlador:" << Ruta_controlador << "\n";
        out << "Ruta aircraft:" << Ruta_aircraft << "\n";
        out << "Numero de puntos de paso:" << Puntos_paso << "\n";
        file.close();

        //QMessageBox::information(this, "Guardar", "Datos guardados correctamente en datos.txt");
    }
        QProcess *process = new QProcess(this);
        QString homeDir = QDir::homePath();
        QString scriptPath = homeDir + "/PprzGCS/Planificacion/Python_sw/build_flight_plan/Código_QT_PYTHON_V1_2.py";
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
            QMessageBox::information(this, "Resultado de la optimización", output);
            qDebug() << "Salida no válida del script Python:" << output;
        }

        process->deleteLater();
}


void PlanificacionWindow::on_button_compilacion_clicked()
{
    disconnect(ui->button_compilacion, &QPushButton::clicked, this, &PlanificacionWindow::on_button_compilacion_clicked);

    // Crear un proceso para ejecutar el script Python
    QProcess *process_compilacion = new QProcess(this);

    QString scriptPath_compilacion = homeDir + "/PprzGCS/Planificacion/Python_sw/build_flight_plan/compilacion_paparazzi.py";

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


void PlanificacionWindow::on_button_datos_clicked()
{
    disconnect(ui->button_datos, &QPushButton::clicked, this, &PlanificacionWindow::on_button_datos_clicked);

    QFile file( homeDir + "/PprzGCS/Planificacion/datos.txt");

    Ruta_mapa = ui->label_mapa->text();
    Ruta_controlador = ui->label_controlador->text();
    Ruta_aircraft = ui->label_aircraft->text();
    Puntos_paso = ui->label_Puntos_paso->text();

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "Estrategia seleccionada: " << EstrategiaSeleccionada << "\n";
        out << "Ruta mapa:" << Ruta_mapa << "\n";
        out << "Ruta controlador:" << Ruta_controlador << "\n";
        out << "Ruta aircraft:" << Ruta_aircraft << "\n";
        out << "Numero de puntos de paso:" << Puntos_paso << "\n";
        file.close();

        QMessageBox::information(this, "Guardar", "Datos guardados correctamente en datos.txt");
    }
}

void PlanificacionWindow::on_button_editor_clicked()
{
    disconnect(ui->button_editor, &QPushButton::clicked, this, &PlanificacionWindow::on_button_editor_clicked);

    QFile file( homeDir + "/PprzGCS/Planificacion/datos.txt");

    Ruta_mapa = ui->label_mapa->text();
    Ruta_controlador = ui->label_controlador->text();
    Ruta_aircraft = ui->label_aircraft->text();
    Puntos_paso = ui->label_Puntos_paso->text();

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "Estrategia seleccionada: " << EstrategiaSeleccionada << "\n";
        out << "Ruta mapa:" << Ruta_mapa << "\n";
        out << "Ruta controlador:" << Ruta_controlador << "\n";
        out << "Ruta aircraft:" << Ruta_aircraft << "\n";
        out << "Numero de puntos de paso:" << Puntos_paso << "\n";
        file.close();

        //QMessageBox::information(this, "Guardar", "Datos guardados correctamente en datos.txt");
    }

    // Crear un proceso para ejecutar el script Python
    QProcess *process_editor = new QProcess(this);

    // Obtener la ruta del directorio home del usuario
    QString homeDir = QDir::homePath();

    QString scriptPath_editor= homeDir + "/PprzGCS/Planificacion/Python_sw/build_flight_plan/open_flight_plan_editor.py";

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
}

void PlanificacionWindow::on_button_abrir_mapa_clicked()
{
    disconnect(ui->button_abrir_mapa, &QPushButton::clicked, this, &PlanificacionWindow::on_button_abrir_mapa_clicked);
    // Abrir el explorador de archivos para seleccionar el archivo de mapa
    QString filePath = QFileDialog::getOpenFileName(this, tr("Abrir archivo de mapa"), QDir::homePath() + "/paparazzi/conf/flight_plans/UCM", tr("Archivos de mapa (*.xml);;Todos los archivos (*)"));

    // Si el usuario selecciona un archivo, mostrar solo el nombre del archivo sin extensión en el QLabel
    if (!filePath.isEmpty()) {
        QFileInfo fileInfo(filePath); // Obtener información del archivo
        QString fileName = fileInfo.completeBaseName(); // Extraer solo el nombre del archivo sin extensión
        ui->label_mapa->setText(fileName); // Mostrar solo el nombre sin extensión en el QLabel
    }
}



void PlanificacionWindow::on_button_abrir_controlador_clicked() // Similar para el archivo de controlador
{
    disconnect(ui->button_abrir_controlador, &QPushButton::clicked, this, &PlanificacionWindow::on_button_abrir_controlador_clicked);
    QString filePath = QFileDialog::getOpenFileName(this, tr("Abrir archivo de controlador"), QDir::homePath() + "/paparazzi/conf/airframes/UCM", tr("Archivos de controlador (*.xml);;Todos los archivos (*)"));

    if (!filePath.isEmpty()) {
        QFileInfo fileInfo(filePath); // Obtener información del archivo
        QString fileName = fileInfo.completeBaseName(); // Extraer solo el nombre del archivo sin extensión
        ui->label_controlador->setText(fileName); // Mostrar solo el nombre en el label
    }
}

int PlanificacionWindow::leerArchivo(const char *filename, int32_t lat[], int32_t lon[], int max_puntos) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error al abrir el archivo");
        return -1;
    }

    int count = 0;
    char nombre[50]; // Para leer el nombre, aunque no lo guardamos
    char linea[256]; // Para leer líneas completas

    // Ignorar la primera línea (encabezado)
    fgets(linea, sizeof(linea), file);

    // Leer cada línea y guardar latitud y longitud
    while (count < max_puntos && fgets(linea, sizeof(linea), file)) {
        if (scanf(linea, "%49s\t%u\t%u", nombre, &lat[count], &lon[count]) == 3) {
                count++;
        }
    }

    fclose(file);
    return count; // Número de puntos leídos
}


void PlanificacionWindow::on_button_move_wp_clicked()
{
    //disconnect(ui->button_move_wp, &QPushButton::clicked, this, &PlanificacionWindow::on_button_move_wp_clicked);
    auto messages = appConfig()->value("MESSAGES").toString();

    dict = new pprzlink::MessageDictionary(messages);

    QString ac_id = "4";
    quint8 wp_id = 8;
    double lat = 39.7910881;
    double lon = -4.0881361;
    float alt = 660;

    pprzlink::Message msg(dict->getDefinition("MOVE_WAYPOINT"));
    msg.setSenderId(pprzlink_id);
    msg.addField("ac_id", ac_id);
    msg.addField("wp_id", wp_id);
    msg.addField("lat", lat);
    msg.addField("long", lon);
    msg.addField("alt", alt);

    if (m_dispatcher) {

        PprzDispatcher::get()->setStart(true);
        qDebug() << "m_dispatcher es no nulo!";
    } else {
        qDebug() << "m_dispatcher es nulo!";
    }
    PprzDispatcher::get()->sendMessage(msg);
}





void PlanificacionWindow::VentanaSector()
{
    QFile file( homeDir + "/PprzGCS/Planificacion/datos.txt");

    Ruta_mapa = ui->label_mapa->text();
    Ruta_controlador = ui->label_controlador->text();
    Ruta_aircraft = ui->label_aircraft->text();
    Puntos_paso = ui->label_Puntos_paso->text();

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "Estrategia seleccionada: " << EstrategiaSeleccionada << "\n";
        out << "Ruta mapa:" << Ruta_mapa << "\n";
        out << "Ruta controlador:" << Ruta_controlador << "\n";
        out << "Ruta aircraft:" << Ruta_aircraft << "\n";
        out << "Numero de puntos de paso:" << Puntos_paso << "\n";
        file.close();

        //QMessageBox::information(this, "Guardar", "Datos guardados correctamente en datos.txt");
    }
    // Obtener la ruta del XML directamente del label_mapa
    QString xmlFilePath = ui->label_mapa->text();  // Este es el contenido del label_mapa

    // Abre la ventana de sectores y pasa la ruta del archivo XML
    sectors_window *ventana = new sectors_window(this);  // Pasa el xmlFilePath al constructor de sectors_window
    ventana->show();
}
