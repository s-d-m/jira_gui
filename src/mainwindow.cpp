#include <iostream>
#include <QAbstractItemView>
#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(std::make_unique<Ui::MainWindow>())
{
    ui->setupUi(this);
    ui->WebView->setHtml(QString("<html><head></head><body><h1>Hello</h1>World!</body></html>"));
//    ui->tableWidget->setContextMenuPolicy(Qt::ContextMenuPolicy::ActionsContextMenu);
    ui->listWidget->setSortingEnabled(true);
    ui->tableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    QObject::connect(ui->issues_list, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(jira_issue_activated(QTreeWidgetItem*,QTreeWidgetItem*)));

}

void MainWindow::jira_issue_activated(QTreeWidgetItem* selected, QTreeWidgetItem* previous_value)
{
    const auto issue_name =  selected->text(0).toStdString();
    ui->WebView->setHtml(QString::fromStdString(std::string("<html><head></head><body><h1>") + issue_name + "</h1>World!</body></html>"));
    ui->tabWidget->setTabText(0, QString::fromStdString(issue_name));
    const auto has_files = (issue_name.starts_with('N'));
    ui->tabWidget->setTabEnabled(2, has_files);
    ui->tabWidget->setCurrentIndex(0);
    std::cout << "jira issue is now " << issue_name << ". Was " << (previous_value ? previous_value->text(0).toStdString() : "nullptr") << std::endl;

    ui->listWidget->clear();
    for (auto i = 0; i < 10; ++i) {
        const auto value = std::format("file for {} numbered {}", issue_name, i);

        ui->listWidget->addItem(QString::fromStdString(value));
    }

    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(40);
    ui->tableWidget->setSortingEnabled(false);
    for (auto i = 0; i < 40; ++i) {
        const auto key = std::format("key for issue {} numbered {}", issue_name, i);
        const auto value= std::format("value is {}", i);

        ui->tableWidget->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(key)));
        ui->tableWidget->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(value)));
    }
    ui->tableWidget->setSortingEnabled(true);

}


void MainWindow::set_some_string(std::string s) {
    ui->WebView->setHtml(QString::fromStdString("<html><head></head><body><verbatim>" + s + "</verbatim></body><html>"));
}
