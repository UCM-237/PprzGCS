#include "sectors_window.h"
#include <QFile>
#include <QTextStream>
#include <QDomDocument>
#include <QMessageBox>
#include <QBoxLayout>
#include <QRegularExpression>
#include <QDir>
#include <QDebug>  // Para depuración

sectors_window::sectors_window(QWidget *parent)
    : QMainWindow(parent), listWidget(new QListWidget(this)) {

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(listWidget);
    QWidget *centralWidget = new QWidget(this);
    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);

    loadXmlFileNameFromTxt();  // Cargar el nombre del archivo XML desde datos.txt
}

sectors_window::~sectors_window() {}

void sectors_window::loadXmlFileNameFromTxt() {
    QString filePath = QDir::homePath() + "/PprzGCS/datos.txt";
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "No se pudo abrir datos.txt.");
        return;
    }

    QTextStream in(&file);
    QString xmlFileName;
    bool foundRutaMapa = false;
    QString planPath = QDir::homePath() + "/paparazzi/flight_plans/UCM/";  // Ruta base donde están los archivos XML

    while (!in.atEnd()) {
        QString line = in.readLine();
        qDebug() << "Leyendo línea: " << line;  // Depuración

        // Verificamos si la línea contiene "Ruta mapa:"
        if (line.startsWith("Ruta mapa:")) {
            // Extraemos todo después de "Ruta mapa:"
            xmlFileName = line.mid(9).trimmed();  // mid(9) elimina "Ruta mapa:" y obtiene el resto
            xmlFileName.append(".xml");  // Agregamos ".xml" al final

            // Añadimos la ruta base para crear la ruta completa
            xmlFileName = planPath + xmlFileName;

            qDebug() << "Ruta completa del XML: " << xmlFileName;  // Depuración: Mostrar la ruta completa

            foundRutaMapa = true;
            break;
        }
    }

    file.close();

    if (!foundRutaMapa) {
        QMessageBox::warning(this, "Error", "No se encontró la ruta del archivo XML en datos.txt.");
            return;
    }

    // Verificar si el archivo XML existe
    if (QFile::exists(xmlFileName)) {
        qDebug() << "El archivo XML existe en: " << xmlFileName;
    } else {
        qDebug() << "El archivo XML no se encuentra en: " << xmlFileName;
    }

    loadWaypoints(xmlFileName);  // Cargar los waypoints desde el archivo XML encontrado
}

void sectors_window::loadWaypoints(const QString &xmlFileName) {
    QFile xmlFile(xmlFileName);
    if (!xmlFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "No se pudo abrir el archivo XML especificado.");
        return;
    }

    QDomDocument doc;
    if (!doc.setContent(&xmlFile)) {
        QMessageBox::warning(this, "Error", "No se pudo analizar el XML.");
        xmlFile.close();
        return;
    }
    xmlFile.close();

    QDomElement root = doc.documentElement();
    QDomNodeList waypoints = root.elementsByTagName("waypoint");

    for (int i = 0; i < waypoints.count(); ++i) {
        QDomNode node = waypoints.at(i);
        if (node.isElement()) {
            QDomElement waypointElement = node.toElement();
            QString name = waypointElement.attribute("name");
            QString lat = waypointElement.attribute("lat");
            QString lon = waypointElement.attribute("lon");

            QString waypointText = QString("Name: %1, Lat: %2, Lon: %3").arg(name, lat, lon);
            listWidget->addItem(waypointText);
        }
    }

    if (waypoints.isEmpty()) {
        QMessageBox::information(this, "Información", "No se encontraron waypoints en el XML.");
    }
}
