#include "transmision_window.h"
#include "ui_transmision_window.h"

#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <QTextStream>
#include <QApplication>
#include <QProcess>
#include <QDebug>



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
    loadFilesFromDirectory(homeDir + "/PprzGCS/Planificacion/JSON", model);

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

    QString nombreArchivo = seleccion.first().data().toString();
    QString rutaCompleta = homeDir + "/PprzGCS/Planificacion/JSON/" + nombreArchivo;

    if (!QFile::exists(rutaCompleta)) {
        QMessageBox::warning(this, "Error", "El archivo no existe:\n" + rutaCompleta);
        return;
    }

    bool exito = QDesktopServices::openUrl(QUrl::fromLocalFile(rutaCompleta));
    if (!exito) {
        QMessageBox::warning(this, "Error", "No se pudo abrir el archivo:\n" + rutaCompleta);
    }
}




void transmision_window::loadFilesFromDirectory(const QString &path, QStringListModel *model, const QStringList &filters) {
    QDir dir(path);
    if (dir.exists()) {
        QStringList archivos;
        if (filters.isEmpty()) {
            archivos = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
        } else {
            archivos = dir.entryList(filters, QDir::Files | QDir::NoDotAndDotDot);
        }
        // Ordenar alfabéticamente (de más antiguo a más reciente)
        archivos.sort();

        // Si quieres invertir para que lo más reciente aparezca arriba
        std::reverse(archivos.begin(), archivos.end());

        model->setStringList(archivos);
    } else {
        model->setStringList(QStringList() << "Directorio no encontrado");
    }
}


