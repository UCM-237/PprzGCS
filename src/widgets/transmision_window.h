#ifndef TRANSMISION_WINDOW_H
#define TRANSMISION_WINDOW_H

#include <QWidget>
#include <QStringListModel>

//Declara la clase .ui
namespace Ui {
class transmision_window;  // debe coincidir con el nombre del formulario en .ui
}

//Declaro mi clase muestreo_window que hereda de QWidget
class transmision_window : public QWidget {
    Q_OBJECT

public:
    //Constructor de mi clase. Es lo que se ejecuta cuando haces muestreo_window* w = new muestreo_window();
    explicit transmision_window(QWidget *parent = nullptr);
    //Destructor de la clase. Se llama automáticamente cuando el objeto se destruye para liberar memoria.
    ~transmision_window();


private slots:
    void on_button_send_clicked();
    void on_button_datos_clicked();

private:
    Ui::transmision_window *ui;
    QStringListModel *model;
    QString homeDir;

    void loadFilesFromDirectory(const QString &path, QStringListModel *model, const QStringList &filters = QStringList());
};


#endif // TRANSMISION_WINDOW_H
