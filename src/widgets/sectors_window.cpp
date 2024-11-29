#include "sectors_window.h"
#include <QFile>
#include <QTextStream>
#include <QDomDocument>
#include <QInputDialog>
#include <QMessageBox>
#include <QBoxLayout>
#include <QRegularExpression>
#include <QDir>
#include <QDebug>  // Para depuración

sectors_window::sectors_window(QWidget *parent)
    : QMainWindow(parent), listWidget(new QListWidget(this)), tableWidget(new QTableWidget(this)),
      button_add_sector(new QPushButton("Add Sector", this)),
      button_remove_sector(new QPushButton("Remove Sector", this)),
      button_save(new QPushButton("Save Sectors", this)),  // Botón para guardar sectores en XML
      columnCount(0) {

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(listWidget);
    layout->addWidget(tableWidget);
    layout->addWidget(button_add_sector);  // Añadir el botón para agregar columnas
    layout->addWidget(button_remove_sector);  // Añadir el botón para eliminar columnas
    layout->addWidget(button_save);  // Añadir el botón para guardar en XML
    QWidget *centralWidget = new QWidget(this);
    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);

    tableWidget->setRowCount(10);  // Número máximo de filas (ajustable)
    tableWidget->setColumnCount(1);  // Inicialmente, solo una columna
    tableWidget->setHorizontalHeaderLabels(QStringList() << "Waypoint Name");

    listWidget->setSelectionMode(QAbstractItemView::MultiSelection);  // Selección múltiple en la lista

    // Conectar los botones
    connect(button_add_sector, &QPushButton::clicked, this, &sectors_window::addSelectedWaypointsToTable);
    connect(button_remove_sector, &QPushButton::clicked, this, &sectors_window::removeColumnFromTable);  // Conectar el botón para eliminar columnas
    connect(button_save, &QPushButton::clicked, this, &sectors_window::saveSectorsToXml);  // Conectar el botón para guardar en XML

    loadXmlFileNameFromTxt();  // Cargar el nombre del archivo XML desde datos.txt
}

sectors_window::~sectors_window() {}

void sectors_window::loadXmlFileNameFromTxt() {
    QString filePath = QDir::homePath() + "/PprzGCS/Planificacion/datos.txt";
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "No se pudo abrir datos.txt.");
        return;
    }

    QTextStream in(&file);
    QString xmlFileName;
    bool foundRutaMapa = false;
    QString planPath = QDir::homePath() + "/paparazzi/conf/flight_plans/UCM/";  // Ruta base donde están los archivos XML

    while (!in.atEnd()) {
        QString line = in.readLine();
        qDebug() << "Leyendo línea: " << line;  // Depuración

        // Verificamos si la línea contiene "Ruta mapa:"
        if (line.startsWith("Ruta mapa:")) {
            // Extraemos todo después de "Ruta mapa:"
            xmlFileName = line.mid(10).trimmed();  // mid(10) elimina "Ruta mapa:" y obtiene el resto
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
    this->xmlFileName = xmlFileName;
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

            QString waypointText = QString("%1").arg(name);  // Solo mostramos el nombre
            listWidget->addItem(waypointText);
        }
    }

    if (waypoints.isEmpty()) {
        QMessageBox::information(this, "Información", "No se encontraron waypoints en el XML.");
    }
}

void sectors_window::addSelectedWaypointsToTable() {
    // Obtener los elementos seleccionados en la lista
    QList<QListWidgetItem*> selectedItems = listWidget->selectedItems();

    if (selectedItems.isEmpty()) {
        QMessageBox::information(this, "Información", "No se ha seleccionado ningún waypoint.");
        return;
    }

    // Crear un cuadro de diálogo para seleccionar la estrategia
    QStringList estrategias = {"ZigZag", "Espiral", "Red"};  // Ejemplo de estrategias
    bool ok;
    estrategiaSeleccionada = QInputDialog::getItem(this, "Seleccionar Estrategia", "Seleccione la estrategia:",
        estrategias, 0, false, &ok);

    Estrategia_vect.append(estrategiaSeleccionada);

    // Crear un cuadro de diálogo para seleccionar el número de puntos por sector
    numPuntos = QInputDialog::getInt(this, "Número de Puntos",
        "Seleccione el número de puntos por sector:", 1, 1, 100, 1, &ok);

    numPuntos_vect.append(numPuntos);


    if (!ok) {
        QMessageBox::information(this, "Información", "No se seleccionó ninguna estrategia.");
        return;
    }

    // Añadir una nueva columna para los puntos seleccionados
    columnCount++;
    tableWidget->insertColumn(columnCount);  // Insertar una nueva columna

    // Establecer el encabezado de la columna (nombre del sector)
    tableWidget->setHorizontalHeaderItem(columnCount, new QTableWidgetItem(estrategiaSeleccionada + QString::number(columnCount)));

    // Añadir los puntos seleccionados a la nueva columna
    for (int i = 0; i < selectedItems.size(); ++i) {
        int row = i;  // Cada nombre va en una nueva fila
        QTableWidgetItem *newItem = new QTableWidgetItem(selectedItems[i]->text());
        tableWidget->setItem(row, columnCount, newItem);  // Coloca el nombre en la nueva columna
    }
}

void sectors_window::removeColumnFromTable() {
    // Eliminar la última columna
    if (columnCount > 0) {
        tableWidget->removeColumn(columnCount);
        columnCount--;
    } else {
        QMessageBox::information(this, "Información", "No hay columnas para eliminar.");
    }
}


void sectors_window::saveSectorsToXml() {
    // Llamamos a la función para cargar el archivo XML desde datos.txt
    loadXmlFileNameFromTxt();

    if (xmlFileName.isEmpty()) {
        QMessageBox::warning(this, "Error", "No se encontró el archivo XML.");
            return;
    }

    QFile file(xmlFileName);

    // Crear un objeto para el documento XML
    QDomDocument doc;

    // Si el archivo XML existe y se puede abrir
    if (file.exists()) {
        if (!doc.setContent(&file)) {
            QMessageBox::warning(this, "Error", "No se pudo analizar el archivo XML.");
            file.close();
            return;
        }
        file.close();  // Cerrar el archivo después de cargarlo
    }

    // Buscar el bloque <sectors> existente
    QDomElement sectors = doc.documentElement().firstChildElement("sectors");

    // Si no existe el bloque <sectors>, crearlo
    if (sectors.isNull()) {
        sectors = doc.createElement("sectors");
        doc.appendChild(sectors);
    }

    // Añadir los nuevos sectores dentro del bloque <sectors>
    int i=1;
    for (int col = 1; col <= columnCount; ++col) {
        QString sectorName = "Sector" + QString::number(col);
        QDomElement sectorElement = doc.createElement("sector");

        QTableWidgetItem* headerItem = tableWidget->horizontalHeaderItem(col);
        QString columnHeader = headerItem ? headerItem->text() : "Unknown";

        sectorElement.setAttribute("color", "red");
        sectorElement.setAttribute("name", columnHeader);
        sectorElement.setAttribute("type", QString::number(numPuntos_vect[col-1]));
        sectors.appendChild(sectorElement);  // Añadir el sector al bloque <sectors>

        for (int row = 0; row < tableWidget->rowCount(); ++row) {
            QTableWidgetItem *item = tableWidget->item(row, col);
            if (item && !item->text().isEmpty()) {
                QDomElement cornerElement = doc.createElement("corner");
                cornerElement.setAttribute("name", item->text());
                sectorElement.appendChild(cornerElement);  // Añadir el corner al sector
            }
        }
        i++;
    }

    // Buscar el nodo <waypoints> en el documento
    QDomElement waypoints = doc.documentElement().firstChildElement("waypoints");

    // Si encontramos el nodo waypoints, insertamos los sectores después de él
    if (!waypoints.isNull()) {
        doc.documentElement().insertAfter(sectors, waypoints);  // Insertar <sectors> después de <waypoints>
    } else {
        // Si no encontramos el nodo <waypoints>, simplemente lo añadimos al final del documento
        doc.appendChild(sectors);
    }

    // Ahora vamos a guardar el archivo XML con los nuevos sectores añadidos
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "No se pudo escribir en el archivo XML.");
        return;
    }

    QTextStream out(&file);
    out << doc.toString();  // Convertir el documento XML a texto y escribirlo en el archivo
    file.close();

    QMessageBox::information(this, "Éxito", "Los sectores han sido guardados en XML.");
}


