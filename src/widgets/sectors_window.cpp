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
      button_add_sector(new QPushButton("Añadir sector", this)),
      button_remove_sector(new QPushButton("Eliminar sector", this)),
      button_save(new QPushButton("Guardar sectores", this)),  // Botón para guardar sectores en XML
      columnCount(0) {

    // Configura el tamaño inicial de la ventana
    this->resize(800, 600); // 800x600 píxeles

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
    loadSectors(xmlFileName);
    loadWaypoints(xmlFileName);  // Cargar los waypoints desde el archivo XML encontrado
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

void sectors_window::loadSectors(const QString &xmlFileName) {
    QFile xmlFile(xmlFileName);
    if (!xmlFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "No se pudo abrir el archivo XML para cargar los sectores.");
        return;
    }

    QDomDocument doc;
    if (!doc.setContent(&xmlFile)) {
        QMessageBox::warning(this, "Error", "No se pudo analizar el XML para cargar los sectores.");
        xmlFile.close();
        return;
    }
    xmlFile.close();

    QDomElement root = doc.documentElement();
    QDomNodeList sectorNodes = root.elementsByTagName("sector");

    for (int i = 0; i < sectorNodes.count(); ++i) {
        QDomNode node = sectorNodes.at(i);
        if (node.isElement()) {
            QDomElement sectorElement = node.toElement();

            QString sectorName = sectorElement.attribute("name");
            QString sectorType = sectorElement.attribute("type");

            // Crear una nueva columna en la tabla para el sector
            columnCount++;
            tableWidget->insertColumn(columnCount);
            tableWidget->setHorizontalHeaderItem(columnCount, new QTableWidgetItem(sectorName));

            // Extraer los corners del sector y añadirlos a la columna
            QDomNodeList corners = sectorElement.elementsByTagName("corner");
            for (int j = 0; j < corners.count(); ++j) {
                QDomNode cornerNode = corners.at(j);
                if (cornerNode.isElement()) {
                    QDomElement cornerElement = cornerNode.toElement();
                    QString cornerName = cornerElement.attribute("name");

                    // Insertar el corner en la fila correspondiente
                    QTableWidgetItem *newItem = new QTableWidgetItem(cornerName);
                    tableWidget->setItem(j, columnCount, newItem);
                }
            }

            // Añadir el tipo de sector a la lista interna (si es necesario)
            Estrategia_vect.append(sectorName);
            numPuntos_vect.append(sectorType.toInt());
            Estrategia_vect.clear();
            numPuntos_vect.clear();
            //columnCount = 0;

        }
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
    QStringList estrategias = {"ZigZag", "Espiral", "Net", "Zona_prohibida"};  // Ejemplo de estrategias
    bool ok;
    estrategiaSeleccionada = QInputDialog::getItem(this, "Tipo de sector", "Seleccione el tipo de sector:",
        estrategias, 0, false, &ok);

    Estrategia_vect.append(estrategiaSeleccionada);

    if (estrategiaSeleccionada != "Net" && estrategiaSeleccionada != "Zona_prohibida"){
        // Crear un cuadro de diálogo para seleccionar el número de puntos por sector
        numPuntos = QInputDialog::getInt(this, "Número de Puntos",
            "Seleccione el número de puntos por sector:", 1, 1, 100, 1, &ok);

        numPuntos_vect.append(numPuntos);
    }

    if (!ok) {
        QMessageBox::information(this, "Información", "No se seleccionó ninguna estrategia.");
        return;
    }

    // Añadir una nueva columna para los puntos seleccionados
    columnCount++;
    tableWidget->insertColumn(columnCount);  // Insertar una nueva columna

    if (estrategiaSeleccionada != "Net" && estrategiaSeleccionada != "Zona_prohibida"){
        // Establecer el encabezado de la columna (nombre del sector)
        tableWidget->setHorizontalHeaderItem(columnCount, new QTableWidgetItem(estrategiaSeleccionada + QString::number(columnCount) + "_" + QString::number(numPuntos)));
    }
    else{
        tableWidget->setHorizontalHeaderItem(columnCount, new QTableWidgetItem(estrategiaSeleccionada + QString::number(columnCount)));
    }

    // Añadir los puntos seleccionados a la nueva columna
    for (int i = 0; i < selectedItems.size(); ++i) {
        int row = i;  // Cada nombre va en una nueva fila
        QTableWidgetItem *newItem = new QTableWidgetItem(selectedItems[i]->text());
        tableWidget->setItem(row, columnCount, newItem);  // Coloca el nombre en la nueva columna
    }
    listWidget->clearSelection();
}

//void sectors_window::removeColumnFromTable() {
//    // Eliminar la última columna
//    if (columnCount > 0) {
//        tableWidget->removeColumn(columnCount);
//        columnCount--;
//    } else {
//        QMessageBox::information(this, "Información", "No hay columnas para eliminar.");
//    }
//}

void sectors_window::removeColumnFromTable() {
    // Obtener la columna seleccionada
    QList<QTableWidgetSelectionRange> selectedRanges = tableWidget->selectedRanges();
    if (!selectedRanges.isEmpty()) {
        int selectedColumn = selectedRanges.first().leftColumn(); // Obtener la primera columna seleccionada
        tableWidget->removeColumn(selectedColumn);
        columnCount--; // Actualizar el contador de columnas
    } else {
        QMessageBox::information(this, "Información", "No hay columnas seleccionadas para eliminar.");
    }
}


//void sectors_window::saveSectorsToXml() {
//    // Llamamos a la función para cargar el archivo XML desde datos.txt
//    loadXmlFileNameFromTxt();

//    if (xmlFileName.isEmpty()) {
//        QMessageBox::warning(this, "Error", "No se encontró el archivo XML.");
//            return;
//    }

//    QFile file(xmlFileName);

//    // Crear un objeto para el documento XML
//    QDomDocument doc;

//    // Si el archivo XML existe y se puede abrir
//    if (file.exists()) {
//        if (!doc.setContent(&file)) {
//            QMessageBox::warning(this, "Error", "No se pudo analizar el archivo XML.");
//            file.close();
//            return;
//        }
//        file.close();  // Cerrar el archivo después de cargarlo
//    }

//    // Buscar el bloque <sectors> existente
//    QDomElement sectors = doc.documentElement().firstChildElement("sectors");

//    // Si no existe el bloque <sectors>, crearlo
//    if (sectors.isNull()) {
//        sectors = doc.createElement("sectors");
//        doc.appendChild(sectors);
//    }

//    // Añadir los nuevos sectores dentro del bloque <sectors>
//    for (int col = 1; col <= columnCount; ++col) {
//        QString sectorName = "Sector" + QString::number(col);
//        QDomElement sectorElement = doc.createElement("sector");

//        QTableWidgetItem* headerItem = tableWidget->horizontalHeaderItem(col);
//        QString columnHeader = headerItem ? headerItem->text() : "Unknown";

//        sectorElement.setAttribute("color", "red");
//        sectorElement.setAttribute("name", columnHeader);
//        sectorElement.setAttribute("type", QString::number(numPuntos_vect[col-1]));
//        sectors.appendChild(sectorElement);  // Añadir el sector al bloque <sectors>

//        for (int row = 0; row < tableWidget->rowCount(); ++row) {
//            QTableWidgetItem *item = tableWidget->item(row, col);
//            if (item && !item->text().isEmpty()) {
//                QDomElement cornerElement = doc.createElement("corner");
//                cornerElement.setAttribute("name", item->text());
//                sectorElement.appendChild(cornerElement);  // Añadir el corner al sector
//            }
//        }
//    }

//    // Buscar el nodo <waypoints> en el documento
//    QDomElement waypoints = doc.documentElement().firstChildElement("waypoints");

//    // Si encontramos el nodo waypoints, insertamos los sectores después de él
//    if (!waypoints.isNull()) {
//        doc.documentElement().insertAfter(sectors, waypoints);  // Insertar <sectors> después de <waypoints>
//    } else {
//        // Si no encontramos el nodo <waypoints>, simplemente lo añadimos al final del documento
//        doc.appendChild(sectors);
//    }

//    // Ahora vamos a guardar el archivo XML con los nuevos sectores añadidos
//    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
//        QMessageBox::warning(this, "Error", "No se pudo escribir en el archivo XML.");
//        return;
//    }

//    QTextStream out(&file);
//    out << doc.toString();  // Convertir el documento XML a texto y escribirlo en el archivo
//    file.close();

//    QMessageBox::information(this, "Éxito", "Los sectores han sido guardados en XML.");
//}

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

    if (!sectors.isNull()) {
        QDomNodeList sectorNodes = sectors.elementsByTagName("sector");

        // Iterar en orden inverso para evitar problemas con la eliminación durante la iteración
        for (int i = sectorNodes.count() - 1; i >= 0; --i) {
            QDomNode node = sectorNodes.at(i);

            // Obtener el nombre del nodo
            QString nodeName = node.nodeName();  // Obtener el nombre del nodo

            // Imprimir el nombre del nodo
            printf("Sector eliminado: %s\n", nodeName.toUtf8().constData());

            // Eliminar el sector del bloque <sectors>
            sectors.removeChild(node);
        }
    }

     else {
        // Si no existe el bloque <sectors>, crearlo
        sectors = doc.createElement("sectors");
        doc.appendChild(sectors);
    }

    // Añadir los nuevos sectores dentro del bloque <sectors>
    for (int col = 1; col <= columnCount; ++col) {
        QString sectorName = "Sector" + QString::number(col);
        QDomElement sectorElement = doc.createElement("sector");

        QTableWidgetItem* headerItem = tableWidget->horizontalHeaderItem(col);
        QString columnHeader = headerItem ? headerItem->text() : "Unknown";

        sectorElement.setAttribute("color", "red");
        sectorElement.setAttribute("name", columnHeader);
        sectors.appendChild(sectorElement);  // Añadir el sector al bloque <sectors>

        for (int row = 0; row < tableWidget->rowCount(); ++row) {
            QTableWidgetItem *item = tableWidget->item(row, col);
            if (item && !item->text().isEmpty()) {
                QDomElement cornerElement = doc.createElement("corner");
                cornerElement.setAttribute("name", item->text());
                sectorElement.appendChild(cornerElement);  // Añadir el corner al sector
            }
        }
        printf("%d\n", col);

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

    //QMessageBox::information(this, "Éxito", "Los sectores han sido guardados en XML.");
    QMessageBox::information(this, "Éxito", "Los sectores han sido guardados en XML.");
}
