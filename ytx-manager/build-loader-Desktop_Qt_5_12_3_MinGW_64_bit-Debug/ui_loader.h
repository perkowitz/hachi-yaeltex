/********************************************************************************
** Form generated from reading UI file 'loader.ui'
**
** Created by: Qt User Interface Compiler version 5.12.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_LOADER_H
#define UI_LOADER_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_FirmwareLoader
{
public:
    QAction *actionOpen;
    QAction *actionSaveAs;
    QAction *actionExit;
    QAction *actionSave;
    QAction *actionFirmware_Update;
    QAction *actionNew;
    QAction *actionAux_Firmware_Update;
    QWidget *centralWidget;
    QVBoxLayout *verticalLayout;
    QVBoxLayout *mainVerticalLayout;
    QHBoxLayout *h1Layout;
    QLabel *label;
    QHBoxLayout *horizontalLayout;
    QLabel *statusLabel;
    QPushButton *sendNotes;
    QMenuBar *menuBar;
    QMenu *menuoctaManager;
    QMenu *optionsMenu;

    void setupUi(QMainWindow *FirmwareLoader)
    {
        if (FirmwareLoader->objectName().isEmpty())
            FirmwareLoader->setObjectName(QString::fromUtf8("FirmwareLoader"));
        FirmwareLoader->resize(462, 341);
        QIcon icon;
        icon.addFile(QString::fromUtf8("logo.png"), QSize(), QIcon::Normal, QIcon::Off);
        FirmwareLoader->setWindowIcon(icon);
        actionOpen = new QAction(FirmwareLoader);
        actionOpen->setObjectName(QString::fromUtf8("actionOpen"));
        actionSaveAs = new QAction(FirmwareLoader);
        actionSaveAs->setObjectName(QString::fromUtf8("actionSaveAs"));
        actionExit = new QAction(FirmwareLoader);
        actionExit->setObjectName(QString::fromUtf8("actionExit"));
        actionSave = new QAction(FirmwareLoader);
        actionSave->setObjectName(QString::fromUtf8("actionSave"));
        actionFirmware_Update = new QAction(FirmwareLoader);
        actionFirmware_Update->setObjectName(QString::fromUtf8("actionFirmware_Update"));
        actionNew = new QAction(FirmwareLoader);
        actionNew->setObjectName(QString::fromUtf8("actionNew"));
        actionAux_Firmware_Update = new QAction(FirmwareLoader);
        actionAux_Firmware_Update->setObjectName(QString::fromUtf8("actionAux_Firmware_Update"));
        centralWidget = new QWidget(FirmwareLoader);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        verticalLayout = new QVBoxLayout(centralWidget);
        verticalLayout->setSpacing(6);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        mainVerticalLayout = new QVBoxLayout();
        mainVerticalLayout->setSpacing(6);
        mainVerticalLayout->setObjectName(QString::fromUtf8("mainVerticalLayout"));
        h1Layout = new QHBoxLayout();
        h1Layout->setSpacing(6);
        h1Layout->setObjectName(QString::fromUtf8("h1Layout"));
        label = new QLabel(centralWidget);
        label->setObjectName(QString::fromUtf8("label"));
        label->setMinimumSize(QSize(0, 100));
        QFont font;
        font.setPointSize(14);
        label->setFont(font);
        label->setAlignment(Qt::AlignCenter);

        h1Layout->addWidget(label);


        mainVerticalLayout->addLayout(h1Layout);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(6);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        statusLabel = new QLabel(centralWidget);
        statusLabel->setObjectName(QString::fromUtf8("statusLabel"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(3);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(statusLabel->sizePolicy().hasHeightForWidth());
        statusLabel->setSizePolicy(sizePolicy);
        statusLabel->setMinimumSize(QSize(50, 100));
        statusLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout->addWidget(statusLabel);


        mainVerticalLayout->addLayout(horizontalLayout);


        verticalLayout->addLayout(mainVerticalLayout);

        sendNotes = new QPushButton(centralWidget);
        sendNotes->setObjectName(QString::fromUtf8("sendNotes"));

        verticalLayout->addWidget(sendNotes);

        FirmwareLoader->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(FirmwareLoader);
        menuBar->setObjectName(QString::fromUtf8("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 462, 21));
        menuoctaManager = new QMenu(menuBar);
        menuoctaManager->setObjectName(QString::fromUtf8("menuoctaManager"));
        optionsMenu = new QMenu(menuBar);
        optionsMenu->setObjectName(QString::fromUtf8("optionsMenu"));
        FirmwareLoader->setMenuBar(menuBar);

        menuBar->addAction(menuoctaManager->menuAction());
        menuBar->addAction(optionsMenu->menuAction());
        menuoctaManager->addSeparator();
        menuoctaManager->addSeparator();
        menuoctaManager->addSeparator();
        menuoctaManager->addAction(actionFirmware_Update);
        menuoctaManager->addAction(actionAux_Firmware_Update);
        menuoctaManager->addSeparator();
        menuoctaManager->addAction(actionExit);

        retranslateUi(FirmwareLoader);

        QMetaObject::connectSlotsByName(FirmwareLoader);
    } // setupUi

    void retranslateUi(QMainWindow *FirmwareLoader)
    {
        FirmwareLoader->setWindowTitle(QApplication::translate("FirmwareLoader", "loader", nullptr));
        actionOpen->setText(QApplication::translate("FirmwareLoader", "&Open From...", nullptr));
        actionSaveAs->setText(QApplication::translate("FirmwareLoader", "Save &As...", nullptr));
        actionSaveAs->setIconText(QApplication::translate("FirmwareLoader", "Save As...", nullptr));
        actionExit->setText(QApplication::translate("FirmwareLoader", "Exit", nullptr));
        actionSave->setText(QApplication::translate("FirmwareLoader", "&Save", nullptr));
        actionSave->setIconText(QApplication::translate("FirmwareLoader", "Save", nullptr));
        actionFirmware_Update->setText(QApplication::translate("FirmwareLoader", "Main Firmware Update", nullptr));
        actionNew->setText(QApplication::translate("FirmwareLoader", "New", nullptr));
        actionAux_Firmware_Update->setText(QApplication::translate("FirmwareLoader", "Aux Firmware Update", nullptr));
        label->setText(QApplication::translate("FirmwareLoader", "Select MIDI ports, then update firmware", nullptr));
        statusLabel->setText(QApplication::translate("FirmwareLoader", "Status", nullptr));
        sendNotes->setText(QApplication::translate("FirmwareLoader", "Send Notes", nullptr));
        menuoctaManager->setTitle(QApplication::translate("FirmwareLoader", "Loader", nullptr));
        optionsMenu->setTitle(QApplication::translate("FirmwareLoader", "&Options", nullptr));
    } // retranslateUi

};

namespace Ui {
    class FirmwareLoader: public Ui_FirmwareLoader {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_LOADER_H
