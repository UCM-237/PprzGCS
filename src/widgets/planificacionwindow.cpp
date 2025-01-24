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
#include<unistd.h>
#include <stdint.h>
#include "gcs_utils.h"
#include <cstdint>
#include <QColor>
#include "waypointeditor.h"
#include "waypoint.h"
#include "waypoint_item.h"
#include "flightplan.h"
#include <QTimer>
#include <QThread>

int currentIndex = 0;  // Índice para seguir el punto actual
QTimer *timer = nullptr;  // Temporizador para el envío de puntos


int i=0;

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
    //connect(ui->button_compilacion, &QPushButton::clicked, this, &PlanificacionWindow::on_button_compilacion_clicked);
    connect(ui->button_editor, &QPushButton::clicked, this, &PlanificacionWindow::on_button_editor_clicked);
    connect(ui->button_sectores, &QPushButton::clicked, this, &PlanificacionWindow::VentanaSector);
    connect(ui->button_datos, &QPushButton::clicked, this, &PlanificacionWindow::on_button_datos_clicked);
//    connect(ui->button_move_wp, &QPushButton::clicked, this, &PlanificacionWindow::on_button_move_wp_clicked);
    connect(ui->button_clear, &QPushButton::clicked, this, &PlanificacionWindow::on_button_clear_clicked);
    connect(ui->button_abrir_mapa, &QPushButton::clicked, this, &PlanificacionWindow::on_button_abrir_mapa_clicked);
    connect(ui->button_abrir_controlador, &QPushButton::clicked, this, &PlanificacionWindow::on_button_abrir_controlador_clicked);
    connect(ui->button_abrir_conf, &QPushButton::clicked, this, &PlanificacionWindow::on_button_abrir_conf_clicked);

    connect(ui->button_move_wp, &QPushButton::clicked, this, &PlanificacionWindow::on_button_move_wp_clicked);

    //Ponemos que en los label ponga por defecto lo que haya escrito en datos.txt
    //En primer lugar leemos cada fila del txt

    // Ruta del archivo (ajusta según la ubicación de tu archivo)

    QFile archivo(homeDir + "/PprzGCS/Planificacion/datos.txt");

    // Verificar si se puede abrir el archivo
    if (!archivo.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "No se pudo abrir el archivo: " + homeDir + "/PprzGCS/Planificacion/datos.txt");
        return;
    }

    QTextStream in(&archivo);
    QStringList lineas;

    // Leer todas las líneas del archivo
    while (!in.atEnd()) {
        lineas.append(in.readLine().trimmed());
    }

    archivo.close();

    // Asignar cada línea a su correspondiente label
    if (lineas.size() > 1) ui->label_mapa->setText(lineas[1].split(":").last().trimmed());
    if (lineas.size() > 2) ui->label_controlador->setText(lineas[2].split(":").last().trimmed());
    if (lineas.size() > 3) ui->label_conf->setText(lineas[3].split(":").last().trimmed());
    if (lineas.size() > 4) ui->label_aircraft->setText(lineas[4].split(":").last().trimmed());
    if (lineas.size() > 5) ui->label_Puntos_paso->setText(lineas[5].split(":").last().trimmed());

    //COnectamos las señales que gestionan las salidas de errores del optimizador
    connect(this, &PlanificacionWindow::errorSignal, this, &PlanificacionWindow::mostrarError);
    connect(this, &PlanificacionWindow::infoSignal, this, &PlanificacionWindow::mostrarInformacion);
    connect(this, &PlanificacionWindow::warningSignal, this, &PlanificacionWindow::mostrarAdvertencia);
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
    QAction action2("Sin Mapa", this);

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

    QFile file(homeDir + "/PprzGCS/Planificacion/datos.txt");

    Ruta_mapa = ui->label_mapa->text();
    Ruta_controlador = ui->label_controlador->text();
    Ruta_aircraft = ui->label_aircraft->text();
    Ruta_conf = ui->label_conf->text();
    Puntos_paso = ui->label_Puntos_paso->text();

    // Intentamos abrir el archivo para escribir
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "Estrategia seleccionada: " << EstrategiaSeleccionada << "\n";
        out << "Ruta mapa:" << Ruta_mapa << "\n";
        out << "Ruta controlador:" << Ruta_controlador << "\n";
        out << "Ruta conf:" << Ruta_conf << "\n";
        out << "Ruta aircraft:" << Ruta_aircraft << "\n";
        out << "Numero de puntos de paso:" << Puntos_paso << "\n";
        file.close();
    } else {
        emit errorSignal("Error", "No se pudo abrir el archivo para escribir: " + file.errorString());
        return;
    }

    if (EstrategiaSeleccionada == " Sin Mapa") {
        emit infoSignal("Estrategia Seleccionada", "Ejecutando la optimización sin mapa");

        // Crea el hilo
        QThread* thread_opt = new QThread(this);

        // Crea un objeto de tipo QObject que contenga el trabajo a hacer
        QObject* worker_opt = new QObject();

        // Conecta la ejecución del script al hilo
        QObject::connect(thread_opt, &QThread::started, worker_opt, [this]() {
            QString scriptPath = homeDir + "/PprzGCS/Planificacion/Python_sw/Move_points/TSP_Regiones_zigzag_espiral_V2.py";

            // Verifica si el archivo del script existe antes de intentar ejecutarlo
            QFile scriptFile(scriptPath);
            if (!scriptFile.exists()) {
                emit errorSignal("Error", "El archivo del script no existe: " + scriptPath);
                return;
            }

            QProcess *process = new QProcess();
            process->start("python", QStringList() << scriptPath);

            // Verifica si el proceso se inicia correctamente
            if (!process->waitForStarted()) {
                emit errorSignal("Error", "No se pudo iniciar el script Python: " + process->errorString());
                process->deleteLater();
                return;
            }

            process->waitForFinished();
            QString output = process->readAllStandardOutput();
            QString errorOutput = process->readAllStandardError();

            // Verifica si hay errores en la salida estándar de error
            if (!errorOutput.isEmpty()) {
                emit errorSignal("Error en la optimización", "Error en la salida del script Python:\n" + errorOutput);
                process->deleteLater();
                return;
            }

            // Procesa la salida estándar del script Python
            if (!output.isEmpty()) {
                // Si la salida contiene información en formato JSON, la procesamos
                QJsonDocument jsonResponse = QJsonDocument::fromJson(output.toUtf8());
                if (!jsonResponse.isNull() && jsonResponse.isObject()) {
                    QJsonObject jsonObj = jsonResponse.object();
                    QString status = jsonObj.value("status").toString();
                    QString message = QString::fromUtf8(jsonObj.value("message").toString().toUtf8());

                    if (status == "success") {
                        emit infoSignal("Ejecución exitosa", "El script de Python se ejecutó correctamente:\n" + message);
                    } else {
                        emit warningSignal("Advertencia", "El script Python reportó un problema:\n" + message);
                    }
                } else {
                    // Si la salida no es JSON, simplemente la mostramos
                    emit infoSignal("Resultado de la optimización", output);
                }
            } else {
                // Si no hay salida estándar, mostramos un mensaje genérico
                emit infoSignal("Resultado de la optimización", "El script Python no produjo salida.");
            }

            process->deleteLater();
        });

        // Conecta el hilo para que el objeto 'worker' se destruya después de ejecutar el trabajo
        QObject::connect(thread_opt, &QThread::finished, worker_opt, &QObject::deleteLater);


        // Mueve el 'worker' al hilo
        worker_opt->moveToThread(thread_opt);

        // Inicia el hilo
        thread_opt->start();

        // Destruye el hilo una vez haya terminado
        QObject::connect(thread_opt, &QThread::finished, thread_opt, &QThread::deleteLater);

    } else if (EstrategiaSeleccionada == " Con Mapa") {
        emit mostrarInformacion("Estrategia Seleccionada", "El optimizador con mapa aún no está desarrollado");
    } else {
        emit mostrarInformacion("Estrategia Seleccionada", "Seleccione una estrategia");
    }
}

// Señales para mensajes
void PlanificacionWindow::mostrarError(const QString &titulo, const QString &mensaje)
{
    QMessageBox::critical(this, titulo, mensaje);
}

void PlanificacionWindow::mostrarInformacion(const QString &titulo, const QString &mensaje)
{
    QMessageBox::information(this, titulo, mensaje);
}

void PlanificacionWindow::mostrarAdvertencia(const QString &titulo, const QString &mensaje)
{
    QMessageBox::warning(this, titulo, mensaje);
}

//void PlanificacionWindow::on_button_compilacion_clicked()
//{
//    disconnect(ui->button_compilacion, &QPushButton::clicked, this, &PlanificacionWindow::on_button_compilacion_clicked);

//    // Crear un proceso para ejecutar el script Python
//    QProcess *process_compilacion = new QProcess(this);

//    QString scriptPath_compilacion = homeDir + "/PprzGCS/Planificacion/Python_sw/build_flight_plan/compilacion_paparazzi.py";

//    // Usa la ruta completa al ejecutable de Python
//    process_compilacion->start("python", QStringList() << scriptPath_compilacion);

//    if (!process_compilacion->waitForStarted()) {
//        qDebug() << "Error al iniciar el script Python:" << process_compilacion->errorString();
//        return;
//    }

//    // Esperar a que el proceso termine
//    process_compilacion->waitForFinished();
//    int exitCode = process_compilacion ->exitCode();
//    QString output = process_compilacion ->readAllStandardOutput();
//    QString errorOutput = process_compilacion ->readAllStandardError();  // Capturar errores

//    // Mostrar la salida y los errores en la consola de depuración
//    qDebug() << "Salida del script Python:" << output;
//    qDebug() << "Error del script Python:" << errorOutput;

//    // Mostrar mensaje de confirmación según el resultado del script Python
//    if (exitCode == 0) {
//        QMessageBox::information(this, "Ejecución exitosa", "El script de Python se ejecutó correctamente.");
//    } else {
//        QMessageBox::warning(this, "Error en la ejecución", "El script de Python finalizó con errores.");
//        qDebug() << "Error en la ejecución del script Python. Código de salida:" << exitCode;
//    }

//    process_compilacion ->deleteLater(); // Eliminar el proceso después de ejecutarse
//    this->close();
//}


void PlanificacionWindow::on_button_datos_clicked()
{
    disconnect(ui->button_datos, &QPushButton::clicked, this, &PlanificacionWindow::on_button_datos_clicked);

    QFile file( homeDir + "/PprzGCS/Planificacion/datos.txt");

    Ruta_mapa = ui->label_mapa->text();
    Ruta_controlador = ui->label_controlador->text();
    Ruta_conf = ui->label_conf->text();
    Ruta_aircraft = ui->label_aircraft->text();
    Puntos_paso = ui->label_Puntos_paso->text();

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "Estrategia seleccionada: " << EstrategiaSeleccionada << "\n";
        out << "Ruta mapa:" << Ruta_mapa << "\n";
        out << "Ruta controlador:" << Ruta_controlador << "\n";
        out << "Ruta conf:" << Ruta_conf << "\n";
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
    Ruta_conf = ui->label_conf->text();
    Puntos_paso = ui->label_Puntos_paso->text();

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "Estrategia seleccionada: " << EstrategiaSeleccionada << "\n";
        out << "Ruta mapa:" << Ruta_mapa << "\n";
        out << "Ruta controlador:" << Ruta_controlador << "\n";
        out << "Ruta conf:" << Ruta_conf << "\n";
        out << "Ruta aircraft:" << Ruta_aircraft << "\n";
        out << "Numero de puntos de paso:" << Puntos_paso << "\n";
        file.close();

        //QMessageBox::information(this, "Guardar", "Datos guardados correctamente en datos.txt");
    }

    // Crear un proceso para ejecutar el script Python
    QProcess *process_editor = new QProcess(this);

    // Obtener la ruta del directorio home del usuario

    QString scriptPath_editor= homeDir + "/PprzGCS/Planificacion/Python_sw/build_flight_plan/open_flight_plan_editor.py";

    // Usa la ruta completa al ejecutable de Python
    process_editor->start("python", QStringList() << scriptPath_editor);

    if (!process_editor->waitForStarted()) {
        qDebug() << "Error al iniciar el script Python:" << process_editor->errorString();
        return;
    }

    // Esperar a que el proceso termine
    process_editor->waitForFinished(3000);
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

    // Directorio base desde donde calcular la ruta relativa
    QString basePath = QDir::homePath() + "/paparazzi/conf/flight_plans";

    // Abrir el explorador de archivos para seleccionar el archivo de mapa
    QString filePath = QFileDialog::getOpenFileName(this, tr("Abrir archivo de mapa"), basePath, tr("Archivos de mapa (*.xml);;Todos los archivos (*)"));

    // Si el usuario selecciona un archivo
    if (!filePath.isEmpty()) {
        QFileInfo fileInfo(filePath); // Obtener información del archivo
        QDir baseDir(basePath); // Crear un objeto QDir con el directorio base
        QString relativePath = baseDir.relativeFilePath(filePath); // Calcular la ruta relativa

        // Mostrar la ruta relativa en el QLabel
        ui->label_mapa->setText(relativePath); // Ejemplo: "UCM/flight_plan_default.xml"
    }
}



void PlanificacionWindow::on_button_abrir_conf_clicked()
{
    disconnect(ui->button_abrir_conf, &QPushButton::clicked, this, &PlanificacionWindow::on_button_abrir_conf_clicked);

    // Directorio base desde donde calcular la ruta relativa
    QString basePath = QDir::homePath() + "/paparazzi/conf/airframes";

    // Abrir el explorador de archivos para seleccionar el archivo de mapa
    QString filePath = QFileDialog::getOpenFileName(this, tr("Abrir archivo de mapa"), QDir::homePath() + "/paparazzi/conf/airframes", tr("Archivos de mapa (*.xml);;Todos los archivos (*)"));

    // Si el usuario selecciona un archivo
    if (!filePath.isEmpty()) {
        QFileInfo fileInfo(filePath); // Obtener información del archivo
        QDir baseDir(basePath); // Crear un objeto QDir con el directorio base
        QString relativePath = baseDir.relativeFilePath(filePath); // Calcular la ruta relativa

        // Mostrar la ruta relativa en el QLabel
        ui->label_conf->setText(relativePath); // Ejemplo: "UCM/flight_plan_default.xml"
    }
}


void PlanificacionWindow::on_button_abrir_controlador_clicked() // Similar para el archivo de controlador
{
    disconnect(ui->button_abrir_controlador, &QPushButton::clicked, this, &PlanificacionWindow::on_button_abrir_controlador_clicked);

    // Directorio base desde donde calcular la ruta relativa
    QString basePath = QDir::homePath() + "/paparazzi/conf/airframes";

    QString filePath = QFileDialog::getOpenFileName(this, tr("Abrir archivo de controlador"), QDir::homePath() + "/paparazzi/conf/airframes", tr("Archivos de controlador (*.xml);;Todos los archivos (*)"));

    // Si el usuario selecciona un archivo
    if (!filePath.isEmpty()) {
        QFileInfo fileInfo(filePath); // Obtener información del archivo
        QDir baseDir(basePath); // Crear un objeto QDir con el directorio base
        QString relativePath = baseDir.relativeFilePath(filePath); // Calcular la ruta relativa

        // Mostrar la ruta relativa en el QLabel
        ui->label_controlador->setText(relativePath); // Ejemplo: "UCM/flight_plan_default.xml"
    }
}

void PlanificacionWindow::VentanaSector()
{
    QFile file( homeDir + "/PprzGCS/Planificacion/datos.txt");

    Ruta_mapa = ui->label_mapa->text();
    Ruta_controlador = ui->label_controlador->text();
    Ruta_aircraft = ui->label_aircraft->text();
    Ruta_conf = ui->label_conf->text();
    Puntos_paso = ui->label_Puntos_paso->text();

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "Estrategia seleccionada: " << EstrategiaSeleccionada << "\n";
        out << "Ruta mapa:" << Ruta_mapa << "\n";
        out << "Ruta controlador:" << Ruta_controlador << "\n";
        out << "Ruta conf:" << Ruta_conf << "\n";
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


void PlanificacionWindow::on_button_move_wp_clicked()
{
    try {
        this->setEnabled(false);  // Deshabilitar toda la ventana
        QMessageBox::information(this, "Moviendo waypoints", "Se va a cargar el flight plan, espere hasta que se haya completado el proceso.");
        bool estado_send_move_wp = true;

        disconnect(ui->button_move_wp, &QPushButton::clicked, this, &PlanificacionWindow::on_button_move_wp_clicked);

        // Obtener la ruta del archivo usando QString
        QString name_flight_plan = ui->label_mapa->text();

        if (name_flight_plan.isEmpty()) {
            throw std::runtime_error("El label del mapa está vacío. No se puede continuar.");
        }

        QFileInfo fileInfo(name_flight_plan); // Usar QFileInfo para manejar la ruta
        const QString filename = homeDir + "/PprzGCS/Planificacion/Resources/waypoints_opt/" + fileInfo.baseName() + "_waypoints.txt"; // Obtener el nombre sin extensión
        qDebug() << "Ruta waypoint: " << filename;

        if (!QFile::exists(filename)) {
            throw std::runtime_error("El archivo de waypoints no existe: " + filename.toStdString());
        }

        double latitudes[100];  // Asegúrate de que el tamaño sea suficiente
        double longitudes[100];
        int max_puntos = 100;    // Número máximo de puntos que leerás del archivo

        // Llamada a la función para leer los puntos del archivo
        int puntos_leidos = leerArchivo(filename.toStdString().c_str(), latitudes, longitudes, max_puntos);
        sendNumwp(puntos_leidos);
        if (puntos_leidos <= 0) {
            throw std::runtime_error("No se pudieron leer puntos del archivo.");
        }

        // Configurar el temporizador para enviar los puntos uno por uno
        currentIndex = 0;  // Reiniciar el índice de los puntos
        timer = new QTimer(this);

        // Conectar la señal timeout del temporizador a una función lambda que maneja el envío de puntos
        connect(timer, &QTimer::timeout, [=]() mutable {
            try {
                if (currentIndex < puntos_leidos) {
                    double latitud = latitudes[currentIndex];
                    double longitud = longitudes[currentIndex];
                    sendwp(latitud, longitud, estado_send_move_wp);  // Llamada a tu función para enviar el waypoint
                    estado_send_move_wp = false;
                    qDebug() << "Waypoint enviado - Latitud:" << latitudes[currentIndex]
                             << " Longitud:" << longitudes[currentIndex]
                             << " Iteración:" << currentIndex;
                    currentIndex++;
                } else {
                    timer->stop();  // Detener el temporizador cuando se hayan enviado todos los puntos
                    timer->deleteLater();
                    this->setEnabled(true);  // Volver a habilitar la ventana
                    qDebug() << "Todos los puntos han sido enviados.";
                }
            } catch (const std::exception &e) {
                qDebug() << "Error durante el envío de waypoints:" << e.what();
                QMessageBox::critical(this, "Error", "Ocurrió un error durante el envío de los waypoints.");
                timer->stop();
                timer->deleteLater();
                this->setEnabled(true);
            }
        });

        // Iniciar el temporizador con un intervalo de 1 segundo (1000 ms)
        timer->start(300);
    } catch (const std::exception &e) {
        qDebug() << "Error en on_button_move_wp_clicked:" << e.what();
        QMessageBox::critical(this, "Error", QString("Ocurrió un error: %1").arg(e.what()));
        this->setEnabled(true);
    } catch (...) {
        qDebug() << "Error desconocido en on_button_move_wp_clicked.";
        QMessageBox::critical(this, "Error", "Ocurrió un error desconocido.");
        this->setEnabled(true);
    }
}


//Función para contar el número de waypoints que hay en el xml cargado
int PlanificacionWindow::countWaypoints(QString &filePath) {
    QFile file(filePath);

    qDebug() << "Archivo xml countWp: " << file;
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "No se pudo abrir el archivo XML en countWaypoints";
        return -1;
    }

    QXmlStreamReader xml(&file);
    int count = 0;

    while (!xml.atEnd() && !xml.hasError()) {
        QXmlStreamReader::TokenType token = xml.readNext();
        if (token == QXmlStreamReader::StartElement && xml.name() == "waypoint") {
            count++;
        }
    }

    if (xml.hasError()) {
        qWarning() << "Error al leer el archivo XML:" << xml.errorString();
    }

    file.close();
    return count;
}


void PlanificacionWindow::on_button_clear_clicked()
{
    this->setEnabled(false);  // Deshabilitar toda la ventana
    QMessageBox::information(this, "Clear", "Se va a ejecutar la limpieza de los waypoints, espere hasta que se haya completado.");
    bool estado_send_conf = 1;
    disconnect(ui->button_clear, &QPushButton::clicked, this, &PlanificacionWindow::on_button_clear_clicked);

    //Primero buscamos que flight_plan está cargado
    QString rutaXml_conf = homeDir + "/paparazzi/conf/airframes/" + ui->label_conf->text(); // Ruta del archivo XML
    QString Aircraft_cargado = ui->label_aircraft->text(); // Nombre a buscar
    QString flight_plan_cargado = ui->label_mapa->text(); // Variable para almacenar el flight_plan encontrado

    qDebug() << "Archivo xml: " << rutaXml_conf;
    // Abrir el archivo XML
    QFile archivo_conf(rutaXml_conf);
    if (!archivo_conf.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "No se pudo abrir el archivo en clear:" << rutaXml_conf;
    }

    // Cargar el contenido del XML
    QDomDocument doc;
    if (!doc.setContent(&archivo_conf)) {
        archivo_conf.close();
        qDebug() << "Error al parsear el archivo XML.";
    }
    archivo_conf.close();

    // Obtener el nodo raíz
    QDomElement root = doc.documentElement();
    if (root.tagName() != "conf") {
        qDebug() << "El archivo XML no tiene el formato esperado.";
    }

    // Buscar el flight_plan asociado al nombre
    QDomNodeList aircraftNodes = root.elementsByTagName("aircraft");
    for (int i = 0; i < aircraftNodes.count(); ++i) {
        QDomElement aircraft = aircraftNodes.at(i).toElement();
        if (!aircraft.isNull()) {
            QString name = aircraft.attribute("name");
            if (name == Aircraft_cargado) {
                //qDebug() << "Buscando el flight_plan";
                flight_plan_cargado = aircraft.attribute("flight_plan");
                break;
            }
        }
    }
    qDebug() << "Flight plan cargado en el conf" << flight_plan_cargado;
    QString ruta_flight_plan_cargado =  homeDir +  "/paparazzi/conf/" + flight_plan_cargado ; // Cambia esta ruta al archivo real

    int num_wp = countWaypoints(ruta_flight_plan_cargado);

    double latitudes_clear[100];  // Asegúrate de que el tamaño sea suficiente
    double longitudes_clear[100];

    // Configurar el temporizador para enviar los puntos uno por uno
    currentIndex = 0;  // Reiniciar el índice de los puntos
    timer = new QTimer(this);

    for(int j = 0; j < num_wp; j++){
        latitudes_clear[j] = 39.7961819;
        longitudes_clear[j] = -4.0758811;
    }

    // Conectar la señal timeout del temporizador a una función lambda que maneja el envío de puntos
    connect(timer, &QTimer::timeout, [=]() mutable{
        if (currentIndex < num_wp) {
            double latitud = latitudes_clear[currentIndex];
            double longitud = longitudes_clear[currentIndex];
            sendwp(latitud, longitud, estado_send_conf);  // Llamada a tu función para enviar el waypoint
            estado_send_conf = 0;
            qDebug() << ": " << latitudes_clear[currentIndex] << ", " << latitudes_clear[currentIndex] << " iteración: " << currentIndex;
                    currentIndex++;
        } else {
            timer->stop();  // Detener el temporizador cuando se hayan enviado todos los puntos
            timer->deleteLater();
            this->setEnabled(true);  // Deshabilitar toda la ventana
            qDebug() << "Todos los puntos han sido enviados.";
        }
    });

    // Iniciar el temporizador con un intervalo de 1 segundo (1000 ms)
    timer->start(300);

    //}
}


int PlanificacionWindow::leerArchivo(const char *filename, double lat[], double lon[], int max_puntos) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error al abrir el archivo");
        return -1;
    }

    int count = 0;
    char linea[256]; // Para leer líneas completas
    char nombre[50];
    char slat[50];
    char slon[50];
    // Ignorar la primera línea (encabezado)
    fgets(linea, sizeof(linea), file);

    // Leer cada línea y parsear los valores
    while (count < max_puntos && fgets(linea, sizeof(linea), file)) {
//        qDebug() << "Leyendo línea: " << linea; // Depuración

        sscanf(linea, "%s\t%s\t%s\n", nombre, slat, slon);

        sscanf(slat, "%lf", &lat[count]);
        sscanf(slon, "%lf", &lon[count]);
//        qDebug() << "Nombre" << nombre <<"count" << count << "Latitud:" << slat << "Longitud:" << slon;
//        qDebug() << "Nombre" << nombre <<"count" << count << "Latitud:" << lat[count] << "Longitud:" << lon[count];


        count++;
    }

    fclose(file);
    return count; // Número de puntos leídos
}



void PlanificacionWindow::sendwp(double latitud, double longitud, bool aux_reset){
    // Recorrer los puntos leídos y enviar un mensaje para cada par de coordenadas
    auto messages = appConfig()->value("MESSAGES").toString();
    dict = new pprzlink::MessageDictionary(messages);


    double lat;
    double lon;
    float alt;
    PprzDispatcher::get()->setStart(true);

    //Se busca el ac_id del vehículo en el archivo conf.xml por el nombre del aircraft que se le haya dado en el datos.txt

    QString rutaXml = homeDir + "/paparazzi/conf/airframes/" + ui->label_conf->text(); // Ruta del archivo XML
    QString nombreBuscado = ui->label_aircraft->text(); // Nombre a buscar
    QString aircraft_id; // Variable para almacenar el ac_id encontrado

    qDebug() << "Archivo xml: " << rutaXml;
    // Abrir el archivo XML
    QFile archivo(rutaXml);
    if (!archivo.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "No se pudo abrir el archivo:" << rutaXml;
    }

    // Cargar el contenido del XML
    QDomDocument doc;
    if (!doc.setContent(&archivo)) {
            archivo.close();
            qDebug() << "Error al parsear el archivo XML.";
    }
    archivo.close();

    // Obtener el nodo raíz
    QDomElement root = doc.documentElement();
    if (root.tagName() != "conf") {
            qDebug() << "El archivo XML no tiene el formato esperado.";
    }

    // Buscar el ac_id asociado al nombre
    QDomNodeList aircraftNodes = root.elementsByTagName("aircraft");
    for (int i = 0; i < aircraftNodes.count(); ++i) {
            QDomElement aircraft = aircraftNodes.at(i).toElement();
            if (!aircraft.isNull()) {
                QString name = aircraft.attribute("name");
                if (name == nombreBuscado) {
                    //qDebug() << "Buscando el ac_id";
                    aircraft_id = aircraft.attribute("ac_id");
                    break;
                }
            }
    }



    // Recorrer cada par de coordenadas
    if (aux_reset == 1){
            i=0;
    }
    quint8 wp_id = i+8; // Puedes usar el índice para asignar un ID de waypoint único
    lat = latitud; // Convertir a formato de latitud/longitud
    lon = longitud; // Convertir a formato de latitud/longitud
    alt = 660.7;
    qDebug() << "aircraft_id = " << aircraft_id;
    qDebug() << "Nombre buscado en XML:" << nombreBuscado;
    pprzlink::Message msg(dict->getDefinition("MOVE_WAYPOINT"));
    msg.setSenderId(pprzlink_id);
    msg.addField("ac_id", aircraft_id);
    msg.addField("wp_id", wp_id);
    msg.addField("lat", lat);
    msg.addField("long", lon);
    msg.addField("alt", alt);


    // Enviar el mensaje para este waypoint

    PprzDispatcher::get()->sendMessage(msg);

    qDebug() << "Enviado waypoint " << wp_id << ": " << lat << ", " << lon << "," << alt << "," << i;
    //printf("Latitud enviada %11.8lf\n", lat);
    i++;

}

//ENVÍO CON CÓDIGO DE PYTHON
//void PlanificacionWindow::sendNumwp(quint8 numWpMoved){
//    // Ruta al script de Python
//    QString scriptPath_num_wp_moved = homeDir + "/paparazzi/sw/ground_segment/python/send_num_wp_moved.py";

//    // Argumentos para pasar al script
//    QStringList arguments;
//    arguments << QString::number(numWpMoved);

//    // Crear un proceso
//    QProcess process_num_wp_moved;

//    // Ejecutar el script con argumentos
//    process_num_wp_moved.start("python3", QStringList() << scriptPath_num_wp_moved << arguments);
//    //process_num_wp_moved.start("python3", QStringList() << scriptPath_num_wp_moved);

//    // Esperar a que el script termine
//    if (!process_num_wp_moved.waitForFinished()) {
//            qDebug() << "Error al ejecutar el script:" << process_num_wp_moved.errorString();
//            return;
//    }

//    // Capturar salida estándar y errores
//    QString output_num_wp_moved = process_num_wp_moved.readAllStandardOutput();
//    QString error_num_wp_moved = process_num_wp_moved.readAllStandardError();

//    qDebug() << "Salida estándar:" << output_num_wp_moved;
//        qDebug() << "Salida de error:" << error_num_wp_moved;
//}

//ENVÍO CON LOS MENSAJES DE LA GCS
void PlanificacionWindow::sendNumwp(quint8 numWpMoved){
        // Recorrer los puntos leídos y enviar un mensaje para cada par de coordenadas
        auto messages = appConfig()->value("MESSAGES").toString();
        dict = new pprzlink::MessageDictionary(messages);

        PprzDispatcher::get()->setStart(true);
        pprzlink::Message msg(dict->getDefinition("NUM_WAYPOINT_MOVED_DATALINK"));
        msg.setSenderId(pprzlink_id);
        msg.addField("num", numWpMoved);

        PprzDispatcher::get()->sendMessage(msg);
}
