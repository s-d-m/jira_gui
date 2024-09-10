#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <iostream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(std::make_unique<Ui::MainWindow>())
{
    ui->setupUi(this);
    QObject::connect(ui->issues_list, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(jira_issue_activated(QTreeWidgetItem*,QTreeWidgetItem*)));

}

void MainWindow::jira_issue_activated(QTreeWidgetItem* selected, QTreeWidgetItem* previous_value)
{
    const auto issue_name =  selected->text(0).toStdString();
    std::cout << "jira issue is now " << issue_name << ". Was " << (previous_value ? previous_value->text(0).toStdString() : "nullptr") << std::endl;
}
