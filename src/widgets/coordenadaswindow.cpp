#include "coordenadaswindow.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>

CoordenadasWindow::CoordenadasWindow(int numCoordenadas, QWidget *parent)
    : QWidget(parent)
{
    layout = new QVBoxLayout(this);  // Crear un layout vertical

    setMinimumSize(700, 600);  // Establecer un tamaño mínimo

    // Crear labels y line edits según el número introducido
    for (int i = 0; i < numCoordenadas; ++i) {
        QLabel *label = new QLabel("Coordenada " + QString::number(i + 1) + ":");
        QLineEdit *lineEdit = new QLineEdit(this);
        lineEdit->setObjectName("lineEdit" + QString::number(i));  // Asignar un nombre a cada QLineEdit

        layout->addWidget(label);
        layout->addWidget(lineEdit);
    }

    // Crear un botón para guardar las coordenadas
    QPushButton *saveButton = new QPushButton("Guardar Coordenadas y cerrar", this);
    layout->addWidget(saveButton);

    // Conectar el botón de guardar a una función que guardará los datos
    connect(saveButton, &QPushButton::clicked, this, [=]() {
        QFile file("coordenadas.txt");
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            for (int i = 0; i < numCoordenadas; ++i) {
                QLineEdit *lineEdit = findChild<QLineEdit *>("lineEdit" + QString::number(i));
                if (lineEdit) {
                    out << lineEdit->text() << "\n";  // Guardar cada coordenada
                }
            }
            file.close();
            close();
            QMessageBox::information(this, "Guardar", "Coordenadas guardadas correctamente.");
        } else {
            QMessageBox::warning(this, "Error", "No se pudo abrir el archivo para guardar las coordenadas.");
        }
    });
}
