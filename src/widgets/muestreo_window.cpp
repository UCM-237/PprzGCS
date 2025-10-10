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
    connect(ui->button_explorer_flight_plan, &QPushButton::clicked, this, &muestreo_window::on_button_explorer_flight_plan_clicked);
    // connect(ui->button_explorer_medidas, &QPushButton::clicked, this, &muestreo_window::on_button_explorer_medidas_clicked);
    connect(ui->button_ver_flight_plan, &QPushButton::clicked, this, &muestreo_window::on_button_open_flight_plan_clicked);
    connect(ui->button_ver_datos_mision, &QPushButton::clicked, this, &muestreo_window::on_button_ver_datos_mision_clicked);
    connect(ui->button_update_csv, &QPushButton::clicked, this, &muestreo_window::on_button_update_csv_clicked);

    //Para que la lista de misiones salga cargado con las misiones que hay en el directorio
    ui->listView_mision->setModel(model);
    loadFilesFromDirectory(homeDir + "/PprzGCS/Planificacion/Resources/logs/nav", model, QStringList() << "*.csv");
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

    // Validación de referencia
    if (Referencia.trimmed().isEmpty()) {
        QMessageBox::warning(this, "Error", "Por favor, introduce una referencia.");
        return;
    }

    // Validación de selección de misión
    QModelIndexList seleccion = ui->listView_mision->selectionModel()->selectedIndexes();
    if (seleccion.isEmpty()) {
        QMessageBox::warning(this, "Error", "Por favor, selecciona una misión.");
        return;
    }
    Mision = seleccion.first().data().toString();
    id_sonda = ui->label_ID_sonda->text();
    valor_ficocianina = ui->label_valor_ficocianina->text();
    std_ficocianina = ui->label_std_ficocianina->text();
    N_ficocianina = ui->label_N_ficocianina->text();
    valor_clorofila = ui->label_valor_clorofila->text();
    std_clorofila = ui->label_std_clorofila->text();
    N_clorofila = ui->label_N_clorofila->text();
    // archivo_medidas = ui->label_archivo_medidas->text();    // Unused
    // periodo_medidas = ui->label_periodo_medidas->text();    // Unused
    incidencias = ui->label_incidencias->toPlainText();
    flight_plan = ui->label_flight_plan->text();
    QFile file(homeDir + "/PprzGCS/Planificacion/Muestreo/" + Referencia + ".txt");

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "Responsable: " << "{" + Responsable + "}" << "\n";
        out << "Lugar: " << "{" + Lugar + "}" << "\n";
        out << "Referencia: " << "{" + Referencia + "}" << "\n";
        out << "Flight_plan: " << "{" + flight_plan + "}" << "\n";
        out << "Mision: " << "{" + Mision + "}" << "\n";
        out << "ID_sonda: " << "{" + id_sonda + "}" << "\n";
        out << "Valor ficocianina: " << "{" + valor_ficocianina + "}" << "\n";
        out << "std ficocianina: " << "{" + std_ficocianina + "}" << "\n";
        out << "Numero medidas ficocianina: " << "{" + N_ficocianina + "}" << "\n";
        out << "Valor clorofila: " << "{" + valor_clorofila + "}" << "\n";
        out << "std clorofila: " << "{" + std_clorofila + "}" << "\n";
        out << "Numero medidas clorofila: " << "{" + N_clorofila + "}" << "\n";
        // out << "Ruta archivo medidas: " << "{" + archivo_medidas + "}" << "\n";
        // out << "Periodo medidas: " << "{" + periodo_medidas + "}" << "\n";
        out << "Incidencias: " << "{" + incidencias + "}" << "\n";

        file.close();

        //QMessageBox::information(this, "Guardar", "Datos guardados correctamente en " + Referencia + ".txt");

        QString nombre_csv = Mision;
        if (nombre_csv.endsWith(".data")) {
            nombre_csv.chop(5);
        }
        else if(nombre_csv.endsWith(".csv")){
            nombre_csv.chop(4);
        }
        if (nombre_csv.startsWith("log_")) {
            nombre_csv = nombre_csv.mid(4);
        }

        const QString jsonFilePath = homeDir + "/PprzGCS/Planificacion/JSON/Barco/" + Referencia + ".geojson";
        const QString csvFilePath = homeDir + "/PprzGCS/Planificacion/Resources/logs/nav/log_" + nombre_csv + ".csv";

        const QString jsonFilePath_sonda = homeDir + "/PprzGCS/Planificacion/JSON/Sonda/" + Referencia + "_sonda.geojson";
        const QString csvFilePath_sonda = homeDir + "/PprzGCS/Planificacion/Resources/logs/sonda/sonda_" + nombre_csv + ".csv";

        extraccion_datos(false, jsonFilePath, csvFilePath, jsonFilePath_sonda, csvFilePath_sonda);
    }
}
void muestreo_window::on_button_explorer_flight_plan_clicked()
{
    disconnect(ui->button_explorer_flight_plan, &QPushButton::clicked, this, &muestreo_window::on_button_explorer_flight_plan_clicked);

    QString basePath = QDir::homePath() + "/paparazzi/conf/flight_plans";
    QString filePath = QFileDialog::getOpenFileName(this, tr("Abrir archivo de muestreo"), basePath);

    if (!filePath.isEmpty()) {
        QDir baseDir(basePath);
        QString relativePath = baseDir.relativeFilePath(filePath);  // Esto te da "UCM/flight_plan.xml"
        ui->label_flight_plan->setText(relativePath);  // Esto es lo que luego usas
    }

}

// void muestreo_window::on_button_explorer_medidas_clicked()
// {
//     disconnect(ui->button_explorer_medidas, &QPushButton::clicked, this, &muestreo_window::on_button_explorer_medidas_clicked);

//     QString basePath = QDir::homePath() + "/PprzGCS/Planificacion/Resources/logs/sonda";
//     QString filePath = QFileDialog::getOpenFileName(this, tr("Abrir archivo de muestreo"), basePath);

//     if (!filePath.isEmpty()) {
//         QDir baseDir(basePath);
//         QString relativePath = baseDir.relativeFilePath(filePath);  // Esto te da "UCM/flight_plan.xml"
//         ui->label_archivo_medidas->setText(filePath);  // Esto es lo que luego usas
//     }

// }

void muestreo_window::on_button_open_flight_plan_clicked()
{

    QString Ruta_flight_plan = ui->label_flight_plan->text();

    // Crear un proceso para ejecutar el script Python
    QProcess *process_editor = new QProcess(this);

    // Obtener la ruta del directorio home del usuario

    QString scriptPath_editor= homeDir + "/PprzGCS/Planificacion/Python_sw/build_flight_plan/open_flight_plan_editor_muestreo.py";

    // Usa la ruta completa al ejecutable de Python
    process_editor->start("python", QStringList() << scriptPath_editor << Ruta_flight_plan);

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
                    else if (key == "Flight_plan") {
                        ui->label_flight_plan->setText(value);
                    }
                    else if (key == "Mision") {
                        QString misionCargada = value;
                        QStringList misionesDisponibles;

                        // Ruta al directorio donde están las misiones
                        QString misionesPath = homeDir + "/PprzGCS/Planificacion/Resources/logs/nav";
                        QDir misionesDir(misionesPath);
                        QStringList filtros;
                        filtros << "*.csv";  // o la extensión real que usas para las misiones
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
                    else if (key == "ID_sonda")
                        ui->label_ID_sonda->setText(value);
                    else if (key == "Valor ficocianina")
                        ui->label_valor_ficocianina->setText(value);
                    else if (key == "std ficocianina")
                        ui->label_std_ficocianina->setText(value);
                    else if (key == "Numero medidas ficocianina")
                        ui->label_N_ficocianina->setText(value);
                    else if (key == "Valor clorofila")
                        ui->label_valor_clorofila->setText(value);
                    else if (key == "std clorofila")
                        ui->label_std_clorofila->setText(value);
                    else if (key == "Numero medidas clorofila")
                        ui->label_N_clorofila->setText(value);
                    // else if (key == "Ruta archivo medidas")
                    //     ui->label_archivo_medidas->setText(value);
                    //  else if (key == "Periodo medidas")
                    //     ui->label_periodo_medidas->setText(value);
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


void muestreo_window::on_button_update_csv_clicked() {

    disconnect(ui->button_update_csv, &QPushButton::clicked, this, &muestreo_window::on_button_update_csv_clicked);

    // Deshabilitar la UI mientras se descarga
    this->setEnabled(false);

    QString logsDir = QDir::homePath() + "/PprzGCS/Planificacion/Resources/logs";

    // Asegurar que el directorio existe
    QDir dir(logsDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QProcess *wgetProcess = new QProcess(this);

    // Comando wget
    QStringList args;
    args << "-r" << "-l2" << "-nH" << "-A" << "*.csv" << "--no-clobber"
         << "--timeout=10" << "--tries=1"
         << "http://192.168.50.1:8080/";
    qDebug() << "Descargando archivos CSV desde la URL 192.168.50.1";


    // Guardar en tu carpeta logs local
    wgetProcess->setWorkingDirectory(logsDir);

    connect(wgetProcess, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
        this, [=](int exitCode, QProcess::ExitStatus) {
        qDebug() << "Proceso wget terminado con código:" << exitCode;
        QStringList files;
        if (exitCode == 0) {
            QMessageBox::information(this, "CSV", "Descarga completada.");
        } else {
            QMessageBox::warning(this, "CSV", "Error en la descarga.");
        }
        // Recargar la lista de misiones después de la descarga
        QString misionesPath = logsDir + "/nav";
        QDir misionesDir(misionesPath);
        QStringList filters;
        filters << "*.csv";
        files = misionesDir.entryList(filters, QDir::Files);
        model->setStringList(files);
        wgetProcess->deleteLater();

        // Volver a habilitar la UI
        this->setEnabled(true);
    });

    wgetProcess->start("/usr/bin/wget", args);

    if (!wgetProcess->waitForStarted()) {
        qDebug() << "wget no arrancó:" << wgetProcess->errorString();
        this->setEnabled(true); // Rehabilitar la UI si falla al arrancar
    }
}



//Función para extraer los datos de la misión ejecutando el .py
void muestreo_window::extraccion_datos(bool mostrarDespues, const QString &jsonFilePath, const QString &csvFilePath, const QString &jsonFilePath_sonda, const QString &csvFilePath_sonda)
{

    //Se le pasa al código de python el nombre del log de la misión
    QModelIndexList selectedIndexes = ui->listView_mision->selectionModel()->selectedIndexes();

    if (selectedIndexes.isEmpty()) {
        QMessageBox::warning(this, "Aviso", "Por favor, selecciona un archivo de la lista de misiones.");
        return;
    }

    QString archivoSeleccionado = selectedIndexes.first().data().toString();
    QString pythonExecutable = "python3";

    //También le tengo que pasar cuál es el archivo de medidas de la sonda -> archivo_medidas
    

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
            this, [this, process, mostrarDespues, jsonFilePath, csvFilePath, jsonFilePath_sonda, csvFilePath_sonda](int exitCode, QProcess::ExitStatus exitStatus) {
                qDebug() << "Proceso terminado con código:" << exitCode;
                process->deleteLater();

                if (mostrarDespues) {
                    mostrar_datos_mision();
                }
                else{
                    guardarVentanaYCsvEnJson(jsonFilePath, csvFilePath);
                    guardarVentanaYCsvEnJson_sonda(jsonFilePath_sonda, csvFilePath_sonda);
                }
            });


    // QString ruta_archivo_medidas = archivo_medidas;
    // if (!QFile::exists(ruta_archivo_medidas)) {
    //     QMessageBox::critical(this, "Error", "No hay ningún archivo de medidas con el nombre " + archivo_medidas + " en ~/PprzGCS/Planificacion/Resources/logs/sonda");
    //         return;
    // }
    // else{
    //     if(!mostrarDespues){
    //         QMessageBox::information(this, "Guardar", "Datos guardados correctamente en " + Referencia + ".geojson");

    //     }
    // }
    // if (archivo_medidas == ""){
    //     archivo_medidas = "empty.csv";
    // }
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

    QString ruta = QDir::homePath() + "/PprzGCS/Planificacion/Resources/logs/nav/" + nombre_csv;

    QFileInfo archivo(ruta);
    if (archivo.exists()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(ruta));
    } else {
        qDebug() << "El archivo no existe:" << ruta;
    }
}


void muestreo_window::guardarVentanaYCsvEnJson(const QString &geoJsonFilePath, const QString &csvFilePath)
{
    QJsonObject geoJsonRoot;
    geoJsonRoot["type"] = "FeatureCollection";
    QJsonArray featuresArray;

    QString nombre_archivo = QFileInfo(csvFilePath).fileName();  // Extrae solo el nombre del csv
    QString home_dir = QDir::homePath();  // o el método que uses en tu proyecto
    QString ruta_csv_procesado = QDir(home_dir + "/PprzGCS/Planificacion/Extraccion_datos/Barco/" + nombre_archivo).absolutePath();

    // Reemplazamos csvFilePath por la ruta corregida
    QFile csvFile(ruta_csv_procesado);

    // qDebug () << "Leyendo CSV desde:" << ruta_csv_procesado;
    if (!csvFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "No se pudo abrir el archivo CSV:" << ruta_csv_procesado;
        return;
    }

    QTextStream in(&csvFile);
    QString headerLine = in.readLine();
    QStringList headers = headerLine.split(',');

    int latIndex = headers.indexOf("lat");
    int lonIndex = headers.indexOf("lon");
    int perfilIndex = headers.indexOf("profile_id");
    
    // int size_properties_nav = 19; // Número de datos correspondientes a la navegación

    if (latIndex == -1 || lonIndex == -1 || perfilIndex == -1) {
        qWarning("No se encontraron columnas 'lat', 'lon' y 'profile_id' en el CSV");
        return;
    }

    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList values = line.split(',');
        if (values.size() != headers.size()) continue;

        double lat = values[latIndex].toDouble();
        double lon = values[lonIndex].toDouble();

        // Crear geometría GeoJSON
        QJsonObject geometry;
        geometry["type"] = "Point";
        QJsonArray coordinates;
        coordinates.append(lon);  // x = longitud
        coordinates.append(lat);  // y = latitud
        geometry["coordinates"] = coordinates;

        // Separar propiedades en bloques
        QJsonObject properties_nav;
        // QJsonObject properties_sonda;

        for (int i = 0; i < headers.size(); ++i) {
            if (i == latIndex || i == lonIndex)
                continue;

            QString val = values[i].trimmed();
            bool isNumber;
            double num = val.toDouble(&isNumber);
            QJsonValue value = isNumber ? QJsonValue(num) : QJsonValue(val);
            properties_nav[headers[i].trimmed()] = value;
        }

        QJsonObject properties;
        properties["navegacion"] = properties_nav;

        QJsonObject feature;
        feature["type"] = "Feature";
        feature["geometry"] = geometry;
        feature["properties"] = properties;

        featuresArray.append(feature);
    }

    qDebug () << "Número de features procesados:" << featuresArray.size();
    csvFile.close();


    geoJsonRoot["features"] = featuresArray;

    // Añadir metadatos de la interfaz gráfica
    QJsonObject metadata;
    metadata["Responsable"] = ui->label_responsable->text();
    metadata["Lugar"] = ui->label_lugar->text();
    metadata["Referencia"] = ui->label_referencia->text();
    metadata["Incidencias"] = ui->label_incidencias->toPlainText();

    QJsonArray misionesArray;
    QModelIndexList selectedIndexes = ui->listView_mision->selectionModel()->selectedIndexes();
    for (const QModelIndex &idx : selectedIndexes) {
        misionesArray.append(model->data(idx).toString());
    }

    metadata["Mision"] = misionesArray;
    metadata["Flight_plan"] = ui->label_flight_plan->text();
    geoJsonRoot["metadata"] = metadata;

    // Guardar GeoJSON
    QJsonDocument doc(geoJsonRoot);
    QFile geoJsonFile(geoJsonFilePath);
    if (geoJsonFile.open(QIODevice::WriteOnly)) {
        geoJsonFile.write(doc.toJson());
        geoJsonFile.close();
    } else {
        qWarning("No se pudo abrir archivo para guardar GeoJSON");
    }
}

void muestreo_window::guardarVentanaYCsvEnJson_sonda(const QString &geoJsonFilePath_sonda, const QString &csvFilePath_sonda)
{
    QJsonObject geoJsonRoot;
    geoJsonRoot["type"] = "FeatureCollection";
    QJsonArray featuresArray;

    QString nombre_archivo = QFileInfo(csvFilePath_sonda).fileName();  // Extrae solo el nombre del csv
    QString home_dir = QDir::homePath();
    QString ruta_csv_procesado = QDir(home_dir + "/PprzGCS/Planificacion/Extraccion_datos/Sonda/" + nombre_archivo).absolutePath();

    QFile csvFile(ruta_csv_procesado);
    if (!csvFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning().noquote().nospace() << "No se pudo abrir el archivo CSV, ruta = {" << ruta_csv_procesado << "}";
        return;
    }

    QTextStream in(&csvFile);
    QString headerLine = in.readLine();
    if (headerLine.trimmed().isEmpty()) {
        qWarning("CSV vacío o sin cabecera");
        csvFile.close();
        return;
    }

    QStringList headers = headerLine.split(',', Qt::KeepEmptyParts);

    // --- Normalización de cabeceras y helpers ---
    auto normalizeKey = [](QString s) {
        s = s.trimmed().toLower();
        s.remove(' ');
        return s;
    };

    QHash<QString,int> idx;              // key normalizada -> índice
    QHash<QString,QString> normToOrig;   // key normalizada -> nombre original

    for (int i = 0; i < headers.size(); ++i) {
        QString orig = headers[i].trimmed();
        QString key = normalizeKey(orig);
        idx.insert(key, i);
        normToOrig.insert(key, orig);
    }

    auto getIndex = [&](const QString &name)->int {
        QString key = normalizeKey(name);
        return idx.contains(key) ? idx.value(key) : -1;
    };

    auto getOrigName = [&](const QString &name)->QString {
        QString key = normalizeKey(name);
        return normToOrig.contains(key) ? normToOrig.value(key) : name;
    };
    // --- fin helpers ---

    // índices relevantes (usando getIndex para tolerar mayúsculas/espacios)
    int fechaUTCIndex = getIndex("fecha_utc");
    int perfilIndex = getIndex("profile_id");
    if (perfilIndex == -1) perfilIndex = getIndex("perfil"); // fallback
    int latIndex = getIndex("lat");
    int lonIndex = getIndex("lon");
    int xIndex = getIndex("x");
    int yIndex = getIndex("y");
    int tIniIndex = getIndex("t_ini");
    int tFinIndex = getIndex("t_fin");
    int zonaUTMIndex = getIndex("utm_zone");

    if (perfilIndex == -1 || latIndex == -1 || lonIndex == -1) {
        qWarning("Faltan columnas obligatorias (profile_id/perfil, lat, lon) en el CSV de sonda");
        csvFile.close();
        return;
    }

    // variables de sonda que queremos agrupar (normalizadas)
    QStringList variablesNorm = {"blue","chl","do","do_sat","ph","profundidad","temperatura"};

    // Agrupar filas por perfil
    QMap<QString, QList<QStringList>> perfilesDatos;
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.trimmed().isEmpty()) continue;
        QStringList values = line.split(',', Qt::KeepEmptyParts);
        while (values.size() < headers.size()) values.append(QString());
        QString perfil = values[perfilIndex].trimmed();
        perfilesDatos[perfil].append(values);
    }
    csvFile.close();

    // Procesar cada perfil
    for (auto it = perfilesDatos.begin(); it != perfilesDatos.end(); ++it) {
        QString perfil = it.key();
        QList<QStringList> filas = it.value();
        if (filas.isEmpty()) continue;

        // Primera fila como referencia del perfil
        const QStringList &first = filas.first();

        QString fecha_ini_utc = (fechaUTCIndex != -1) ? first[fechaUTCIndex].trimmed() : QString();
        QString t_ini = (tIniIndex != -1) ? first[tIniIndex].trimmed() : QString();
        QString t_fin = (tFinIndex != -1) ? first[tFinIndex].trimmed() : QString();
        QString zona_utm = (zonaUTMIndex != -1) ? first[zonaUTMIndex].trimmed() : QString();

        bool ok;
        double lat = (latIndex != -1) ? first[latIndex].toDouble(&ok) : 0.0;
        double lon = (lonIndex != -1) ? first[lonIndex].toDouble(&ok) : 0.0;
        double x = (xIndex != -1) ? first[xIndex].toDouble(&ok) : 0.0;
        double y = (yIndex != -1) ? first[yIndex].toDouble(&ok) : 0.0;

        // Geometría (Point)
        QJsonObject geometry;
        geometry["type"] = "Point";
        QJsonArray coordinates;
        coordinates.append(lon);
        coordinates.append(lat);
        geometry["coordinates"] = coordinates;

        // Propiedades de navegación del perfil (constantes)
        QJsonObject properties_nav;
        properties_nav["fecha_ini_utc"] = fecha_ini_utc;
        properties_nav["name"] = QString("Punto_%1").arg(perfil);
        properties_nav["perfil"] = perfil;
        properties_nav["t_fin"] = t_fin;
        properties_nav["t_ini"] = t_ini;
        properties_nav["x"] = x;
        properties_nav["y"] = y;
        double zval = zona_utm.toDouble(&ok);
        if (ok) properties_nav["zona_utm"] = zval;
        else properties_nav["zona_utm"] = zona_utm;

        // Construir arrays para cada variable de sonda
        QJsonObject properties_sonda;
        for (const QString &varNorm : variablesNorm) {
            int varIndex = getIndex(varNorm);
            if (varIndex == -1) continue; // no existe esa columna
            QString origName = getOrigName(varNorm); // nombre original para la clave JSON

            QJsonArray valoresArray;
            for (const QStringList &fila : filas) {
                QString cell = fila[varIndex].trimmed();
                // tratar 'nan' como NULL
                if (cell.isEmpty() || cell.toLower() == "nan") {
                    valoresArray.append(QJsonValue::Null);
                    continue;
                }
                bool ok2;
                double v = cell.toDouble(&ok2);
                if (ok2) valoresArray.append(v);
                else valoresArray.append(QJsonValue(cell)); // si no es numérico, lo añadimos como string
            }
            properties_sonda[origName] = valoresArray;
        }

        // Montar properties final
        QJsonObject properties;
        properties["navegacion"] = properties_nav;
        properties["sonda"] = properties_sonda;

        // Feature completo
        QJsonObject feature;
        feature["type"] = "Feature";
        feature["geometry"] = geometry;
        feature["properties"] = properties;

        featuresArray.append(feature);
    }

    geoJsonRoot["features"] = featuresArray;

    // Metadatos (igual que antes)
    QJsonObject metadata;
    metadata["Referencia"] = ui->label_referencia->text();
    metadata["ID_sonda"] = ui->label_ID_sonda->text();
    metadata["Valor ficocianina"] = ui->label_valor_ficocianina->text();
    metadata["std ficocianina"] = ui->label_std_ficocianina->text();
    metadata["Numero medidas ficocianina"] = ui->label_N_ficocianina->text();
    metadata["Valor clorofila"] = ui->label_valor_clorofila->text();
    metadata["std clorofila"] = ui->label_std_clorofila->text();
    metadata["Numero medidas clorofila"] = ui->label_N_clorofila->text();
    // metadata["Ruta archivo medidas"] = ui->label_archivo_medidas->text();
    // metadata["Periodo medidas"] = ui->label_periodo_medidas->text();
    geoJsonRoot["metadata"] = metadata;

    // Guardar GeoJSON
    QJsonDocument doc(geoJsonRoot);
    QFile geoJsonFile(geoJsonFilePath_sonda);
    if (!geoJsonFile.open(QIODevice::WriteOnly)) {
        qWarning("No se pudo abrir archivo para guardar GeoJSON");
        return;
    }
    geoJsonFile.write(doc.toJson(QJsonDocument::Indented));
    geoJsonFile.close();
}
