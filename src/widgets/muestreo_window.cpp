#include "muestreo_window.h"
#include "ui_muestreo_window.h"

#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <QTextStream>
#include <QFileDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QProcess>
#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

muestreo_window::muestreo_window(QWidget *parent) :
    QWidget(parent), //Llama al constructor padre QWidget(parent)
    ui(new Ui::muestreo_window), //Crea el puntero a la interfaz muestreo_window 
    model(new QStringListModel(this))
    {
    ui->setupUi(this);
    ui->label_incidencias->setLineWrapMode(QTextEdit::WidgetWidth);
    setWindowTitle("Muestreo");
    homeDir = QDir::homePath(); //Directorio home
    connect(ui->button_save, &QPushButton::clicked, this, &muestreo_window::on_button_save_clicked);  //Botón para guardar
    connect(ui->button_explorer_referencia, &QPushButton::clicked, this, &muestreo_window::on_button_explorer_referencia_clicked);  //Explorador de archivos
    connect(ui->button_ver_datos_mision, &QPushButton::clicked, this, &muestreo_window::on_button_ver_datos_mision_clicked);  
    //Para que la lista de misiones salga cargado con las misiones que hay en el directorio
    ui->listView_mision->setModel(model);
    loadFilesFromDirectory(homeDir + "/paparazzi/var/logs", model, QStringList() << "*.data");
}

muestreo_window::~muestreo_window() {
    delete ui;
}

void muestreo_window::loadFilesFromDirectory(const QString &path, QStringListModel *model, const QStringList &filters) {
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

void muestreo_window::on_button_save_clicked()
{
    disconnect(ui->button_save, &QPushButton::clicked, this, &muestreo_window::on_button_save_clicked);

    Responsable = ui->label_responsable->text();
    Lugar = ui->label_lugar->text();
    Referencia = ui->label_referencia->text();
    QModelIndexList seleccion = ui->listView_mision->selectionModel()->selectedIndexes();
    if (!seleccion.isEmpty()) {
        Mision = seleccion.first().data().toString();
    }
    h_inicio_ficocianina = ui->label_h_inicio_ficocianina->text();
    h_fin_ficocianina = ui->label_h_fin_ficocianina->text();
    h_inicio_clorofila = ui->label_h_inicio_clorofila->text();
    h_fin_clorofila = ui->label_h_fin_clorofila->text();
    archivo_calibracion = ui->label_archivo_calibracion->text();
    archivo_medidas = ui->label_archivo_medidas->text();
    periodo_medidas = ui->label_periodo_medidas->text();
    incidencias = ui->label_incidencias->toPlainText();

    QFile file( homeDir + "/PprzGCS/Planificacion/Muestreo/" + Referencia + ".txt");

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "Responsable: " << "{" + Responsable + "}"<< "\n";
        out << "Lugar: " << "{" + Lugar + "}"<< "\n";
        out << "Referencia: " << "{" + Referencia + "}"<< "\n";
        out << "Mision: " << "{" + Mision + "}" << "\n";
        out << "Hora inicio ficocianina: " << "{" + h_inicio_ficocianina + "}"<< "\n";
        out << "Hora fin ficocianina: " << "{" + h_fin_ficocianina + "}"<< "\n";
        out << "Hora inicio clorofila: " << "{" + h_inicio_clorofila + "}"<< "\n";
        out << "Hora fin clorofila: " << "{" + h_fin_clorofila + "}"<< "\n";
        out << "Ruta archivo calibracion: " << "{" + archivo_calibracion + "}"<< "\n";
        out << "Ruta archivo medidas: " << "{" + archivo_medidas + "}"<< "\n";
        out << "Periodo medidas: " << "{" + periodo_medidas + "}"<< "\n";
        out << "Incidencias: " << "{" + incidencias + "}"<< "\n";

        file.close();

        QMessageBox::information(this, "Guardar", "Datos guardados correctamente en " +  Referencia + ".txt");
        // Reemplaza ".data" por ".csv" en Mision
        QString nombre_csv = Mision;
        if (nombre_csv.endsWith(".data")) {
            nombre_csv.chop(5);  // elimina ".data"
            nombre_csv += ".csv";
        }

        const QString jsonFilePath = homeDir + "/PprzGCS/Planificacion/JSON/" + Referencia + ".JSON";
        const QString csvFilePath = homeDir + "/PprzGCS/Planificacion/Extraccion_datos/" + nombre_csv;

        qDebug() << "Ruta CSV: " << csvFilePath;
        extraccion_datos(false, jsonFilePath, csvFilePath);

    }
}


void muestreo_window::on_button_explorer_referencia_clicked()
{
    // Desconectar el botón solo si es necesario
    disconnect(ui->button_explorer_referencia, &QPushButton::clicked, this, &muestreo_window::on_button_explorer_referencia_clicked);

    QString basePath = QDir::homePath() + "/PprzGCS/Planificacion/Muestreo";
    QString filePath = QFileDialog::getOpenFileName(this, tr("Abrir archivo de muestreo"), basePath);

    if (!filePath.isEmpty()) {
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            bool insideIncidencias = false;
            QString incidenciasTexto;

            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();

                // Procesar campo incidencias multilínea
                if (line.startsWith("Incidencias: {")) {
                    insideIncidencias = true;
                    incidenciasTexto = line.mid(line.indexOf('{') + 1);
                    if (incidenciasTexto.endsWith("}")) {
                        incidenciasTexto.chop(1); // eliminar '}'
                        insideIncidencias = false;
                    }
                    continue;
                }

                if (insideIncidencias) {
                    if (line.endsWith("}")) {
                        QString lastLine = line.left(line.length() - 1);
                        incidenciasTexto += "\n" + lastLine;
                        insideIncidencias = false;
                    } else {
                        incidenciasTexto += "\n" + line;
                    }
                    continue;
                }

                // Procesar líneas con formato Clave: {Valor}
                QRegularExpression re("^(.*?):\\s*\\{(.*)\\}$");
                QRegularExpressionMatch match = re.match(line);
                if (match.hasMatch()) {
                    QString key = match.captured(1).trimmed();
                    QString value = match.captured(2).trimmed();

                    if (key == "Responsable")
                        ui->label_responsable->setText(value);
                    else if (key == "Lugar")
                        ui->label_lugar->setText(value);
                    else if (key == "Referencia") {
                        ui->label_referencia->setText(value);
                    }
                    else if (key == "Mision") {
                        QString misionCargada = value;
                        QStringList misionesDisponibles;

                        // Ruta al directorio donde están las misiones
                        QString misionesPath = homeDir + "/paparazzi/var/logs";
                        QDir misionesDir(misionesPath);
                        QStringList filtros;
                        filtros << "*.data";  // o la extensión real que usas para las misiones
                        misionesDisponibles = misionesDir.entryList(filtros, QDir::Files);

                        // Quitar la ruta para comparar solo por nombre
                        model->setStringList(misionesDisponibles);

                        // Establecer el modelo en el listView (por si acaso no estaba)
                        ui->listView_mision->setModel(model);

                        // Buscar y seleccionar la misión cargada
                        for (int i = 0; i < misionesDisponibles.size(); ++i) {
                            if (misionesDisponibles[i] == misionCargada) {
                                QModelIndex index = model->index(i);
                                ui->listView_mision->setCurrentIndex(index);
                                break;
                            }
                        }
                    }

                    else if (key == "Hora inicio ficocianina")
                        ui->label_h_inicio_ficocianina->setText(value);
                    else if (key == "Hora fin ficocianina")
                        ui->label_h_fin_ficocianina->setText(value);
                    else if (key == "Hora inicio clorofila")
                        ui->label_h_inicio_clorofila->setText(value);
                    else if (key == "Hora fin clorofila")
                        ui->label_h_fin_clorofila->setText(value);
                    else if (key == "Ruta archivo calibracion")
                        ui->label_archivo_calibracion->setText(value);
                    else if (key == "Ruta archivo medidas")
                        ui->label_archivo_medidas->setText(value);
                     else if (key == "Periodo medidas")
                        ui->label_periodo_medidas->setText(value);
                }
            }

            file.close();

            // Finalmente, ponemos el texto de incidencias en el widget correspondiente
            ui->label_incidencias->setPlainText(incidenciasTexto);
        }
    }
}



//Lee los datos de la misión y extrae el CSV

void muestreo_window::on_button_ver_datos_mision_clicked()
{
    disconnect(ui->button_ver_datos_mision, &QPushButton::clicked, this, &muestreo_window::on_button_ver_datos_mision_clicked);
    extraccion_datos(true);
}

//Función para extraer los datos de la misión ejecutando el .py
void muestreo_window::extraccion_datos(bool mostrarDespues, const QString &jsonFilePath, const QString &csvFilePath)
{

    QModelIndexList selectedIndexes = ui->listView_mision->selectionModel()->selectedIndexes();

    if (selectedIndexes.isEmpty()) {
        QMessageBox::warning(this, "Aviso", "Por favor, selecciona un archivo de la lista de misiones.");
        return;
    }

    QString archivoSeleccionado = selectedIndexes.first().data().toString();
    QString pythonExecutable = "python3";
    QString scriptPath = homeDir + "/PprzGCS/Planificacion/Python_sw/Extraccion_datos/Extraccion_datos.py";

    QProcess *process = new QProcess(this);

    connect(process, &QProcess::readyReadStandardOutput, [process]() {
        QByteArray output = process->readAllStandardOutput();
        qDebug() << "Output:" << output;
    });

    connect(process, &QProcess::readyReadStandardError, [process]() {
        QByteArray error = process->readAllStandardError();
        qDebug() << "Error:" << error;
    });

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, process, mostrarDespues, jsonFilePath, csvFilePath](int exitCode, QProcess::ExitStatus exitStatus) {
                qDebug() << "Proceso terminado con código:" << exitCode;
                process->deleteLater();

                if (mostrarDespues) {
                    mostrar_datos_mision();
                }
                else{
                    guardarVentanaYCsvEnJson(jsonFilePath, csvFilePath);
                }
            });

    process->start(pythonExecutable, QStringList() << scriptPath << archivoSeleccionado);
}


//Para mostrar los datos de la misión en el csv

void muestreo_window::mostrar_datos_mision()
{
    QString seleccionado = ui->listView_mision->currentIndex().data().toString();

    if (seleccionado.isEmpty())
        return;

    // Reemplaza ".data" por ".csv"
    QString nombre_csv = seleccionado;
    if (nombre_csv.endsWith(".data")) {
        nombre_csv.chop(5);  // elimina ".data"
        nombre_csv += ".csv";
    }

    QString ruta = QDir::homePath() + "/PprzGCS/Planificacion/Extraccion_datos/" + nombre_csv;

    QFileInfo archivo(ruta);
    if (archivo.exists()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(ruta));
    } else {
        qDebug() << "El archivo no existe:" << ruta;
    }
}


void muestreo_window::guardarVentanaYCsvEnJson(const QString &jsonFilePath, const QString &csvFilePath)
{
    QJsonObject jsonRoot;
    // Leer CSV y meterlo en JSON
    QFile csvFile(csvFilePath);
    if (csvFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&csvFile);
        QString headerLine = in.readLine();  // Leer cabecera
        QStringList headers = headerLine.split(',');

        QJsonArray csvArray;
        while (!in.atEnd()) {
            QString line = in.readLine();
            QStringList values = line.split(',');

            QJsonObject rowObj;
            for (int i = 0; i < headers.size() && i < values.size(); ++i) {
                rowObj[headers[i].trimmed()] = values[i].trimmed();
            }
            csvArray.append(rowObj);
        }
        jsonRoot["DatosCSV"] = csvArray;
        csvFile.close();
    } else {
        qWarning("No se pudo abrir el archivo CSV");
    }
    // Guardar datos desde widgets de la ventana:
    jsonRoot["Responsable"] = ui->label_responsable->text();
    jsonRoot["Lugar"] = ui->label_lugar->text();
    jsonRoot["Referencia"] = ui->label_referencia->text();

    // Mision desde QListView y modelo QStringListModel
    QJsonArray misionesArray;
    for (int i = 0; i < model->rowCount(); ++i) {
        QModelIndex idx = model->index(i);
        misionesArray.append(model->data(idx).toString());
    }
    jsonRoot["Mision"] = misionesArray;

    jsonRoot["Hora inicio ficocianina"] = ui->label_h_inicio_ficocianina->text();
    jsonRoot["Hora fin ficocianina"] = ui->label_h_fin_ficocianina->text();
    jsonRoot["Hora inicio clorofila"] = ui->label_h_inicio_clorofila->text();
    jsonRoot["Hora fin clorofila"] = ui->label_h_fin_clorofila->text();

    jsonRoot["Ruta archivo calibracion"] = ui->label_archivo_calibracion->text();
    jsonRoot["Ruta archivo medidas"] = ui->label_archivo_medidas->text();
    jsonRoot["Periodo medidas"] = ui->label_periodo_medidas->text();
    jsonRoot["Incidencias"] = ui->label_incidencias->toPlainText();

    // Guardar JSON
    QJsonDocument doc(jsonRoot);
    QFile jsonFile(jsonFilePath);
    if (jsonFile.open(QIODevice::WriteOnly)) {
        jsonFile.write(doc.toJson());
        jsonFile.close();
    } else {
        qWarning("No se pudo abrir archivo para guardar JSON");
    }
}
