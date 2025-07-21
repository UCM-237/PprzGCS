#include "transmision_window.h"
#include "ui_transmision_window.h"

#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <QTextStream>
#include <QApplication>
#include <QProcess>
#include <QDebug>
#include <QMap>


transmision_window::transmision_window(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::transmision_window),
    model(new QStringListModel(this))

{
    ui->setupUi(this);
    setWindowTitle("Transmision");
    homeDir = QDir::homePath(); //Directorio home
    // Conectar botón (tu código original)
    connect(ui->button_transmitir, &QPushButton::clicked, this, &transmision_window::on_button_send_clicked);
    connect(ui->button_datos, &QPushButton::clicked, this, &transmision_window::on_button_datos_clicked);

    ui->listView_ID->setModel(model);

    // Para que se abra directamnete la Qlist con los archivos del directorio
    loadFilesFromDirectory(homeDir + "/PprzGCS/Planificacion/JSON/Barco/", model);

}

transmision_window::~transmision_window() {
    delete ui;
}

void transmision_window::on_button_send_clicked()
{
    //disconnect(ui->button_transmitir, &QPushButton::clicked, this, &transmision_window::on_button_send_clicked);
    
    QString input = ui->label_contrasena->text();
    
    if (input == "1234"){

        QModelIndexList selectedIndexes = ui->listView_ID->selectionModel()->selectedIndexes();

        if (selectedIndexes.isEmpty()) {
            QMessageBox::warning(this, "Aviso", "Por favor, selecciona un archivo de la lista de IDs.");
            return;
        }

        QString archivoSeleccionado = selectedIndexes.first().data().toString();
        QString pythonExecutable = "python3";
        QString scriptPath = homeDir + "/PprzGCS/Planificacion/Python_sw/Subida_datos/post_request.py";

        QProcess *process = new QProcess(this);

        connect(process, &QProcess::readyReadStandardOutput, [process]() {
            QByteArray output = process->readAllStandardOutput();
            qDebug() << "Output:" << output;
        });

        connect(process, &QProcess::readyReadStandardError, [process]() {
            QByteArray error = process->readAllStandardError();
            qDebug() << "Error:" << error;
        });


        process->start(pythonExecutable, QStringList() << scriptPath << archivoSeleccionado);
    }

    else{
        QMessageBox::critical(this, "Contraseña", "Contraseña incorrecta");
    }
    
}


#include <QDesktopServices>
#include <QUrl>

void transmision_window::on_button_datos_clicked()
{
    disconnect(ui->button_datos, &QPushButton::clicked, this, &transmision_window::on_button_datos_clicked);

    QModelIndexList seleccion = ui->listView_ID->selectionModel()->selectedIndexes();
    if (seleccion.isEmpty()) {
        QMessageBox::warning(this, "Error", "No hay ningún elemento seleccionado en la lista.");
            return;
    }

    QString nombreBase = seleccion.first().data().toString();  // Sin extensión
    QString rutaBarco = filePathMap.value(nombreBase);  // ✅ Ruta completa del archivo Barco
    if (rutaBarco.isEmpty()) {
        QMessageBox::warning(this, "Error", "No se pudo encontrar la ruta del archivo seleccionado.");
        return;
    }

    // Abrir el archivo de Barco
    bool exitoBarco = QDesktopServices::openUrl(QUrl::fromLocalFile(rutaBarco));
    if (!exitoBarco) {
        QMessageBox::warning(this, "Error", "No se pudo abrir el archivo:\n" + rutaBarco);
    }

    // Construir y abrir el archivo de Sonda
    QString rutaSonda = homeDir + "/PprzGCS/Planificacion/JSON/Sonda/" + nombreBase + "_sonda.geojson";

    if (!QFile::exists(rutaSonda)) {
        QMessageBox::warning(this, "Advertencia", "El archivo de sonda no existe:\n" + rutaSonda);
        return;
    }

    bool exitoSonda = QDesktopServices::openUrl(QUrl::fromLocalFile(rutaSonda));
    if (!exitoSonda) {
        QMessageBox::warning(this, "Error", "No se pudo abrir el archivo de sonda:\n" + rutaSonda);
    }
}




//void transmision_window::loadFilesFromDirectory(const QString &path, QStringListModel *model, const QStringList &filters) {
//    QDir dir(path);
//    if (dir.exists()) {
//        QStringList archivos;
//        if (filters.isEmpty()) {
//            archivos = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
//        } else {
//            archivos = dir.entryList(filters, QDir::Files | QDir::NoDotAndDotDot);
//        }
//        // Ordenar alfabéticamente (de más antiguo a más reciente)
//        archivos.sort();

//        // Si quieres invertir para que lo más reciente aparezca arriba
//        std::reverse(archivos.begin(), archivos.end());

//        model->setStringList(archivos);
//    } else {
//        model->setStringList(QStringList() << "Directorio no encontrado");
//    }
//}

void transmision_window::loadFilesFromDirectory(const QString &path, QStringListModel *model, const QStringList &filters) {
    QDir dir(path);
    filePathMap.clear();  // Limpiamos el mapa antes de cargar nuevos archivos

    if (dir.exists()) {
        QStringList archivosSinExtension;

        QStringList archivos;
        if (filters.isEmpty()) {
            archivos = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
        } else {
            archivos = dir.entryList(filters, QDir::Files | QDir::NoDotAndDotDot);
        }

        // Ordenar alfabéticamente (de más antiguo a más reciente)
        archivos.sort();
        std::reverse(archivos.begin(), archivos.end());  // Más reciente arriba

        for (const QString& fileName : archivos) {
            QFileInfo fileInfo(dir.absoluteFilePath(fileName));
            QString nombreSinExtension = fileInfo.completeBaseName();

            archivosSinExtension << nombreSinExtension;
            filePathMap[nombreSinExtension] = fileInfo.absoluteFilePath();  // Guardamos la ruta completa
        }

        model->setStringList(archivosSinExtension);
    } else {
        model->setStringList(QStringList() << "Directorio no encontrado");
    }
}



