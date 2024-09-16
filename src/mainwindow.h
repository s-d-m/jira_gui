#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "qtreewidget.h"
#include <QMainWindow>
#include "ui_mainwindow.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;
    ~MainWindow() override = default;

private slots:
    auto jira_issue_activated(QTreeWidgetItem* selected, QTreeWidgetItem* previous_value) -> void;

public slots:
    auto set_some_string(std::string s) -> void;

private:
    std::unique_ptr<Ui::MainWindow> ui;
};
#endif // MAINWINDOW_H
