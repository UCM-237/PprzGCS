#ifndef SECTORS_WINDOW_H
#define SECTORS_WINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QFile>
#include <QTextStream>
#include <QDomDocument>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QAbstractItemView>

class sectors_window : public QMainWindow
{
    Q_OBJECT

public:
    explicit sectors_window(QWidget *parent = nullptr);
    ~sectors_window();

private slots:
    void loadXmlFileNameFromTxt();  // Cargar el nombre del archivo XML desde datos.txt
    void loadWaypoints(const QString &xmlFileName);  // Cargar waypoints desde el archivo XML
    void addSelectedWaypointsToTable();  // Añadir los waypoints seleccionados a la tabla
    void removeColumnFromTable();  // Eliminar una columna de la tabla
    void saveSectorsToXml();  // Guardar los sectores en un archivo XML

private:
    QListWidget *listWidget;  // Lista para mostrar los waypoints
    QTableWidget *tableWidget;  // Tabla para mostrar los sectores
    QPushButton *button_add_sector;  // Botón para añadir un sector
    QPushButton *button_remove_sector;  // Botón para eliminar un sector
    QPushButton *button_save;  // Botón para guardar los sectores
    QString estrategiaSeleccionada;
    int columnCount;  // Número de columnas en la tabla
    QString xmlFileName;
};

#endif // SECTORS_WINDOW_H

