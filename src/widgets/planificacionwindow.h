#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QMenu>
#include <QProcess>  // Para ejecutar scripts de Python
#include "mapwidget.h"
#include "pprz_dispatcher.h"
#include <pprzlinkQt/Message.h>
#include "AircraftManager.h"
#include "waypoint.h"
#include "waypointeditor.h"
//#include "movewpopt.h"
#include "waypoint_item.h"
#include "flightplan.h"
#include "pprzpalette.h"

QT_BEGIN_NAMESPACE
namespace Ui { class PlanificacionWindow; }
QT_END_NAMESPACE

class PlanificacionWindow : public QMainWindow
{
    Q_OBJECT

public:
    PlanificacionWindow(QWidget *parent = nullptr);
    ~PlanificacionWindow();
    //void sendMessage(pprzlink::Message);
    pprzlink::MessageDictionary* getDict() {return dict;}
    pprzlink::IvyQtLink* link;

signals:
    void move_waypoint_ui(Waypoint*, QString ac_id);
    void waypoint_moved(pprzlink::Message message);
    
private slots:
    void sendwp(double latitud, double longitud, bool aux_reset);
    void sendNumwp(quint8 numWpMoved);
    void on_button_estrategia_clicked();
    void on_button_optimizacion_clicked();   
    //void on_button_compilacion_clicked(); 
    void on_button_editor_clicked();  
    void on_button_datos_clicked(); 
    void on_button_move_wp_clicked();
    void on_button_abrir_controlador_clicked();
    void on_button_abrir_mapa_clicked();
    void on_button_abrir_conf_clicked();
    void on_button_clear_clicked();
    int countWaypoints(QString &filePath);
    void VentanaSector();
    int leerArchivo(const char *filename, double lat[], double lon[], int max_puntos);
    void mostrarError(const QString &titulo, const QString &mensaje);
    void mostrarInformacion(const QString &titulo, const QString &mensaje);
    void mostrarAdvertencia(const QString &titulo, const QString &mensaje);
private:
    Ui::PlanificacionWindow *ui;
    int i=0;
    WaypointItem* wi;
    QString ac_id;
    bool started;
    pprzlink::MessageDictionary* dict;
    QString EstrategiaSeleccionada;
    QString homeDir;
    QString Ruta_mapa;
    QString Ruta_controlador;
    QString Ruta_conf;
    QString Ruta_aircraft;
    QString Puntos_paso;
    QProcess *process;  // Para manejar el proceso del script Python
    MapWidget* mapWidget;
    QString pprzlink_id;
    QList<long> _bindIds;
    void ejecutarScriptPython();  // Método para ejecutar el script y manejar su salida
    QString aircraft_name;
    QStringList lineas;
    bool estado_send_move_wp;
    bool estado_send_conf;
    PprzDispatcher *m_dispatcher;  // Puntero al dispatcher
    
signals:
    // Señales para pasar los mensajes al hilo principal
    void errorSignal(const QString &titulo, const QString &mensaje);
    void infoSignal(const QString &titulo, const QString &mensaje);
    void warningSignal(const QString &titulo, const QString &mensaje);
    
};

#endif // MAINWINDOW_H

