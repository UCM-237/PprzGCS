#ifndef COORDENADASWINDOW_H
#define COORDENADASWINDOW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

class CoordenadasWindow : public QWidget
{
    Q_OBJECT

public:
    explicit CoordenadasWindow(int numCoordenadas, QWidget *parent = nullptr);

private:
    QVBoxLayout *layout;  // Layout vertical para contener los labels y line edits
};

#endif // COORDENADASWINDOW_H
