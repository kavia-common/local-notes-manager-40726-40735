#ifndef WELCOME_H
#define WELCOME_H

#include <QWidget>

class QLabel;

class WelcomeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WelcomeWidget(QWidget *parent = nullptr);
};

#endif // WELCOME_H