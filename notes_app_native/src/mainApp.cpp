#include "mainApp.h"
#include <QApplication>
#include <QLabel>
#include <QVBoxLayout>

WelcomeWidget::WelcomeWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    
    auto *label = new QLabel("Welcome to Native Qt6 Template!", this);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("QLabel { font-size: 18px; font-weight: bold; padding: 20px; }");
    
    auto *subLabel = new QLabel("This is a minimal Qt6 application using CMake.", this);
    subLabel->setAlignment(Qt::AlignCenter);
    subLabel->setStyleSheet("QLabel { font-size: 14px; padding: 10px; }");
    
    layout->addWidget(label);
    layout->addWidget(subLabel);
    
    setWindowTitle("Qt6 Welcome");
    resize(400, 200);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    WelcomeWidget welcome;
    welcome.show();
    
    return app.exec();
}
