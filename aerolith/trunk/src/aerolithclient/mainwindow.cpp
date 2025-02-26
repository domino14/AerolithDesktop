//    Copyright 2007, 2008, 2009, Cesar Del Solar  <delsolar@gmail.com>
//    This file is part of Aerolith.
//
//    Aerolith is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    Aerolith is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Aerolith.  If not, see <http://www.gnu.org/licenses/>.

#include "mainwindow.h"
#include "commonDefs.h"
#include "databasehandler.h"

const quint16 MAGIC_NUMBER = 25349;


bool highScoresLessThan(const tempHighScoresStruct& a, const tempHighScoresStruct& b)
{
    if (a.numCorrect == b.numCorrect) return (a.timeRemaining > b.timeRemaining);
    else return (a.numCorrect > b.numCorrect);
}

MainWindow::MainWindow(QString aerolithVersion, DatabaseHandler* databaseHandler) :
        aerolithVersion(aerolithVersion), dbHandler(databaseHandler), PLAYERLIST_ROLE(Qt::UserRole),
        out(&block, QIODevice::WriteOnly)
{



    createScrambleTableDialog = new QDialog(this);
    uiScrambleTable.setupUi(createScrambleTableDialog);

    createTaxesTableDialog = new QDialog(this);
    uiTaxesTable.setupUi(createTaxesTableDialog);


    uiMainWindow.setupUi(this);
    uiMainWindow.roomTableWidget->verticalHeader()->hide();
    uiMainWindow.roomTableWidget->setColumnWidth(0, 30);
    uiMainWindow.roomTableWidget->setColumnWidth(1, 40);
    uiMainWindow.roomTableWidget->setColumnWidth(2, 200);
    uiMainWindow.roomTableWidget->setColumnWidth(3, 350);
    uiMainWindow.roomTableWidget->setColumnWidth(4, 75);

    uiMainWindow.roomTableWidget->horizontalHeader()->setResizeMode(QHeaderView::Fixed);

    WindowTitle = "Aerolith " + aerolithVersion;
    setWindowTitle(WindowTitle);

    challengesMenu = new QMenu;

    connect(challengesMenu, SIGNAL(triggered(QAction*)), this, SLOT(dailyChallengeSelected(QAction*)));
    uiMainWindow.pushButtonChallenges->setMenu(challengesMenu);

    connect(uiMainWindow.pushButtonNewTable, SIGNAL(clicked()), this, SLOT(createNewRoom()));
    uiMainWindow.chatText->setOpenExternalLinks(true);
    connect(uiMainWindow.listWidgetPeopleConnected, SIGNAL(sendPM(QString)), this, SLOT(sendPM(QString)));
    connect(uiMainWindow.listWidgetPeopleConnected, SIGNAL(viewProfile(QString)), this, SLOT(viewProfile(QString)));

    uiMainWindow.chatText->document()->setMaximumBlockCount(5000);  // at most 5000 newlines.

    connect(uiMainWindow.lineEditChat, SIGNAL(returnPressed()), this, SLOT(submitChatLEContents()));

    // commsSocket

    commsSocket = new QTcpSocket(this);
    connect(commsSocket, SIGNAL(readyRead()), this, SLOT(readFromServer()));
    connect(commsSocket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(displayError(QAbstractSocket::SocketError)));
    connect(commsSocket, SIGNAL(connected()), this, SLOT(connectedToServer()));

    connect(commsSocket, SIGNAL(disconnected()), this, SLOT(serverDisconnection()));
    connect(commsSocket, SIGNAL(bytesWritten(qint64)), this, SLOT(socketWroteBytes(qint64)));

    connect(uiScrambleTable.buttonBox, SIGNAL(accepted()), SLOT(createUnscrambleGameTable()));
    connect(uiScrambleTable.buttonBox, SIGNAL(rejected()), createScrambleTableDialog, SLOT(hide()));

    connect(uiScrambleTable.spinBoxWL, SIGNAL(valueChanged(int)), SLOT(spinBoxWordLengthChange(int)));

    scoresDialog = new QDialog(this);
    uiScores.setupUi(scoresDialog);
    uiScores.scoresTableWidget->verticalHeader()->hide();
    connect(uiScores.pushButtonGetScores, SIGNAL(clicked()), this, SLOT(getScores()));

    loginDialog = new QDialog(this);
    uiLogin.setupUi(loginDialog);
    //  loginDialog->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint);
    connect(uiLogin.loginPushButton, SIGNAL(clicked()), this, SLOT(toggleConnectToServer()));

    connect(uiLogin.registerPushButton, SIGNAL(clicked()), this, SLOT(showRegisterPage()));
    connect(uiLogin.cancelRegPushButton, SIGNAL(clicked()), this, SLOT(showLoginPage()));

    connect(uiLogin.submitRegPushButton, SIGNAL(clicked()), this, SLOT(registerName()));
    connect(uiLogin.pushButtonStartOwnServer, SIGNAL(clicked()), this, SLOT(startOwnServer()));


    scoresDialog->setAttribute(Qt::WA_QuitOnClose, false);
    loginDialog->setAttribute(Qt::WA_QuitOnClose, false);


    gameStarted = false;
    blockSize = 0;

    currentTablenum = 0;
    uiLogin.stackedWidget->setCurrentIndex(0);


    gameBoardWidget = new UnscrambleGameTable(0, Qt::Window, dbHandler);
    gameBoardWidget->setWindowTitle("Table");
    gameBoardWidget->setWindowFlags(Qt::WindowMinMaxButtonsHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint);
    gameBoardWidget->setAttribute(Qt::WA_QuitOnClose, false);

    connect(gameBoardWidget, SIGNAL(giveUp()), this, SLOT(giveUpOnThisGame()));
    connect(gameBoardWidget, SIGNAL(sendStartRequest()), this, SLOT(submitReady()));
    connect(gameBoardWidget, SIGNAL(avatarChange(quint8)), this, SLOT(changeMyAvatar(quint8)));
    connect(gameBoardWidget, SIGNAL(correctAnswerSubmitted(quint8, quint8)), this, SLOT(submitCorrectAnswer(quint8, quint8)));
    connect(gameBoardWidget, SIGNAL(chatTable(QString)), this, SLOT(chatTable(QString)));

    connect(gameBoardWidget, SIGNAL(viewProfile(QString)), this, SLOT(viewProfile(QString)));
    connect(gameBoardWidget, SIGNAL(sendPM(QString)), this, SLOT(sendPM(QString)));
    connect(gameBoardWidget, SIGNAL(exitThisTable()), this, SLOT(leaveThisTable()));


    out.setVersion(QDataStream::Qt_4_2);

    QTime midnight(0, 0, 0);
    qsrand(midnight.msecsTo(QTime::currentTime()));



    connect(uiMainWindow.actionAerolith_Help, SIGNAL(triggered()), this, SLOT(aerolithHelpDialog()));
    connect(uiMainWindow.actionConnect_to_Aerolith, SIGNAL(triggered()), loginDialog, SLOT(raise()));
    connect(uiMainWindow.actionConnect_to_Aerolith, SIGNAL(triggered()), loginDialog, SLOT(show()));
    connect(uiMainWindow.actionAcknowledgements, SIGNAL(triggered()), this, SLOT(aerolithAcknowledgementsDialog()));
    connect(uiMainWindow.actionAbout_Qt, SIGNAL(triggered()), this, SLOT(showAboutQt()));
    connect(uiMainWindow.actionPaypal_Donation, SIGNAL(triggered()), this, SLOT(showDonationPage()));


    gameTimer = new QTimer();
    connect(gameTimer, SIGNAL(timeout()), this, SLOT(updateGameTimer()));

    readWindowSettings();

    show();
    loginDialog->show();
    //  loginDialog->activateWindow();
    loginDialog->raise();
    uiLogin.usernameLE->setFocus(Qt::OtherFocusReason);
    setWindowIcon(QIcon(":images/aerolith.png"));


    setProfileWidget = new QWidget(this, Qt::Window);
    uiSetProfile.setupUi(setProfileWidget);

    getProfileWidget = new QWidget(this, Qt::Window);
    uiGetProfile.setupUi(getProfileWidget);

    databaseDialog = new QDialog(this);
    uiDatabase.setupUi(databaseDialog);

    connect(uiMainWindow.actionCreate_databases, SIGNAL(triggered()), databaseDialog, SLOT(show()));

    connect(uiDatabase.pushButtonCreateDatabases, SIGNAL(clicked()), SLOT(createDatabasesOKClicked()));

    connect(dbHandler, SIGNAL(setProgressMessage(QString)), uiDatabase.labelProgress, SLOT(setText(QString)));
     connect(dbHandler, SIGNAL(setProgressValue(int)), uiDatabase.progressBar, SLOT(setValue(int)));
    connect(dbHandler, SIGNAL(setProgressRange(int, int)), uiDatabase.progressBar, SLOT(setRange(int, int)));
    connect(dbHandler, SIGNAL(enableClose(bool)), SLOT(dbDialogEnableClose(bool)));
    connect(dbHandler, SIGNAL(createdDatabase(QString)), SLOT(databaseCreated(QString)));

    //   connect(uiTable.pushButtonUseOwnList, SIGNAL(clicked()), SLOT(uploadOwnList()));


    wordFilter = new WordFilter(this, Qt::Window);
    connect(uiMainWindow.actionWord_Filter, SIGNAL(triggered()), wordFilter, SLOT(showWidget()));

    wordFilter->setDbHandler(dbHandler);




    ///////
    // set game icons
    unscrambleGameIcon.addFile(":images/unscrambleGameSmall.png");

    existingLocalDBList = dbHandler->checkForDatabases();

    if (existingLocalDBList.size() == 0)
    {
        uiLogin.pushButtonStartOwnServer->setEnabled(false);
        if (QMessageBox::question(this, "Create databases?", "You do not seem to have any lexicon databases created. "
                                  "Creating a lexicon database is necessary to use Aerolith!",
                                  QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
            databaseDialog->show();

    }
    else
    {
        QMenu* rebuildDbMenu = new QMenu(this);
        foreach (QString str, existingLocalDBList)
        {
            setCheckbox(str);
            rebuildDbMenu->addAction(str);
        }
        uiDatabase.pushButtonRebuildDatabase->setMenu(rebuildDbMenu);
        connect(rebuildDbMenu, SIGNAL(triggered(QAction*)), SLOT(rebuildDatabaseAction(QAction*)));
        dbHandler->connectToDatabases(true, existingLocalDBList);
    }

    uiScrambleTable.tableWidgetMyLists->setSelectionMode(QAbstractItemView::SingleSelection);
    uiScrambleTable.tableWidgetMyLists->setSelectionBehavior(QAbstractItemView::SelectRows);
    uiScrambleTable.tableWidgetMyLists->setEditTriggers(QAbstractItemView::NoEditTriggers);


    connect(gameBoardWidget, SIGNAL(saveCurrentGame()),
            SLOT(saveGame()));

    connect(gameBoardWidget, SIGNAL(sitDown(quint8)), SLOT(trySitting(quint8)));
    connect(gameBoardWidget, SIGNAL(standUp()), SLOT(standUp()));
    connect(gameBoardWidget, SIGNAL(setTablePrivate(bool)), SLOT(trySetTablePrivate(bool)));
    connect(gameBoardWidget, SIGNAL(showInviteDialog()), SLOT(showInviteDialog()));
    connect(gameBoardWidget, SIGNAL(bootFromTable(QString)), SLOT(bootFromTable(QString)));

    testFunction(); // used for debugging

     uiLogin.portLE->setText(QString::number(DEFAULT_PORT));
}

void MainWindow::dbDialogEnableClose(bool e)
{
    if (!e)
        uiMainWindow.chatText->append("<font color=red>Databases are being created.</font>");
    if (e)
    {
        uiMainWindow.chatText->append("<font color=red>Lexicon databases were successfully created!</font>");
        QStringList dbList = dbHandler->checkForDatabases();
        dbHandler->connectToDatabases(true, dbList);
        uiLogin.pushButtonStartOwnServer->setEnabled(true);
    }
}

void MainWindow::databaseCreated(QString lexiconName)
{
    setCheckbox(lexiconName);
}

void MainWindow::rebuildDatabaseAction(QAction* action)
{
    if (QMessageBox::question(this, "Rebuild?", "Would you really like to rebuild the database '" + action->text() + "'?",
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
        dbHandler->createLexiconDatabases(QStringList(action->text()));
}

void MainWindow::setCheckbox(QString lexiconName)
{
    if (lexiconName == "OWL2+LWL")
    {
        uiDatabase.checkBoxOWL2->setChecked(true);
        uiDatabase.checkBoxOWL2->setEnabled(false);
    }
    else if (lexiconName == "CSW")
    {
        uiDatabase.checkBoxCSW->setChecked(true);
        uiDatabase.checkBoxCSW->setEnabled(false);
    }
    else if (lexiconName == "FISE")
    {
        uiDatabase.checkBoxFISE->setChecked(true);
        uiDatabase.checkBoxFISE->setEnabled(false);
    }
    else if (lexiconName == "Volost")
    {
        uiDatabase.checkBoxVolost->setChecked(true);
        uiDatabase.checkBoxVolost->setEnabled(false);
    }
    if (!existingLocalDBList.contains(lexiconName))
        existingLocalDBList.append(lexiconName);
}

void MainWindow::createDatabasesOKClicked()
{
    QStringList databasesToCreate;


    if (uiDatabase.checkBoxOWL2->isEnabled() && uiDatabase.checkBoxOWL2->isChecked())
        databasesToCreate << "OWL2+LWL";
    if (uiDatabase.checkBoxCSW->isEnabled() && uiDatabase.checkBoxCSW->isChecked())
        databasesToCreate << "CSW";
    if (uiDatabase.checkBoxFISE->isEnabled() && uiDatabase.checkBoxFISE->isChecked())
        databasesToCreate << "FISE";
    if (uiDatabase.checkBoxVolost->isEnabled() && uiDatabase.checkBoxVolost->isChecked())
        databasesToCreate << "Volost";
    dbHandler->createLexiconDatabases(databasesToCreate);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    writeWindowSettings();
    event->accept();
}

void MainWindow::writeWindowSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                       "CesarWare", "Aerolith");
    settings.beginGroup("MainWindow");
    settings.setValue("pos", pos());
    settings.setValue("username", uiLogin.usernameLE->text());
    if (uiLogin.checkBoxSavePassword->isChecked())
    {
        settings.setValue("password", uiLogin.passwordLE->text());
    }
    else
    {
        settings.setValue("password", "");
    }
    settings.setValue("savePassword", uiLogin.checkBoxSavePassword->isChecked());
    settings.endGroup();

}

void MainWindow::readWindowSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                       "CesarWare", "Aerolith");
    settings.beginGroup("MainWindow");
    move(settings.value("pos", QPoint(100, 100)).toPoint());
    uiLogin.usernameLE->setText(settings.value("username", "").toString());
    uiLogin.passwordLE->setText(settings.value("password", "").toString());
    uiLogin.checkBoxSavePassword->setChecked(settings.value("savePassword", true).toBool());
    settings.endGroup();
}

void MainWindow::writeHeaderData()
{
    out.device()->seek(0);
    block.clear();
    out << (quint16)MAGIC_NUMBER;	// magic number
    out << (quint16)0; // length
}

void MainWindow::fixHeaderLength()
{
    out.device()->seek(sizeof(quint16));
    out << (quint16)(block.size() - sizeof(quint16) - sizeof(quint16));
}

void MainWindow::submitChatLEContents()
{

    /* else if (chatLE->text().indexOf("/me ") == 0)
        {
        writeHeaderData();
        out << (quint8)'a';
        out << chatLE->text().mid(4);
        fixHeaderLength();
        commsSocket->write(block);
        chatLE->clear();
        }*/

    writeHeaderData();
    out << (quint8)CLIENT_CHAT;
    out << uiMainWindow.lineEditChat->text();
    fixHeaderLength();
    commsSocket->write(block);
    uiMainWindow.lineEditChat->clear();
}


void MainWindow::chatTable(QString textToSend)
{
    if (textToSend.indexOf("/me ") == 0)
    {
        writeHeaderData();
        out << (quint8)CLIENT_TABLE_COMMAND;
        out << (quint16)currentTablenum;
        out << (quint8)CLIENT_TABLE_ACTION;
        out << textToSend.mid(4);
        fixHeaderLength();
        commsSocket->write(block);
    }
    else
    {


        writeHeaderData();
        out << (quint8)CLIENT_TABLE_COMMAND;
        out << (quint16)currentTablenum;
        out << (quint8)CLIENT_TABLE_CHAT;
        out << textToSend;
        fixHeaderLength();
        commsSocket->write(block);
    }

}

void MainWindow::submitCorrectAnswer(quint8 space, quint8 specificAnswer)
{
    // chatText->append(QString("From solution: ") + solutionLE->text());
    // solutionLE->clear();

    writeHeaderData();
    out << (quint8) CLIENT_TABLE_COMMAND;
    out << (quint16) currentTablenum;
    out << (quint8) CLIENT_TABLE_UNSCRAMBLEGAME_CORRECT_ANSWER;
    out << space << specificAnswer;
    fixHeaderLength();
    commsSocket->write(block);
}

void MainWindow::changeMyAvatar(quint8 avatarID)
{

    writeHeaderData();
    out << (quint8) CLIENT_TABLE_COMMAND;
    out << (quint16) currentTablenum;
    out << (quint8) CLIENT_TABLE_AVATAR;
    out << avatarID;
    fixHeaderLength();
    commsSocket->write(block);
}


void MainWindow::readFromServer()
{
    // same structure as server's read

    //QDataStream needs EVERY byte to be available for reading!
    while (commsSocket->bytesAvailable() > 0)
    {
        if (blockSize == 0)
        {
            if (commsSocket->bytesAvailable() < 4)
                return;

            quint16 header;
            quint16 packetlength;

            // there are 4 bytes available. read them in!

            in >> header;
            in >> packetlength;
            if (header != (quint16)MAGIC_NUMBER) // magic number
            {
#ifdef Q_WS_MAC
                QMessageBox::critical(this, "Wrong version of Aerolith", "You seem to be using an outdated "
                                      "version of Aerolith. Please go to www.aerolith.org and download the newest "
                                      " update.");
#endif
#ifdef Q_WS_WIN
                // call an updater program
                // QDesktopServices::openUrl(QUrl("http://www.aerolith.org"));
                // QCoreApplication::quit();
                QMessageBox::critical(this, "Wrong version of Aerolith", "You seem to be using an outdated "
                                      "version of Aerolith. Please go to www.aerolith.org and download the newest "
                                      " update.");
#endif
#ifdef Q_WS_X11
                QMessageBox::critical(this, "Wrong version of Aerolith", "You seem to be using an outdated "
                                      "version of Aerolith. If you compiled this from source, please check out the new "
                                      "version of Aerolith by typing into a terminal window: <BR>"
                                      "svn co svn://www.aerolith.org/aerolith/tags/1.0 aerolith<BR>"
                                      "and then go to this directory, type in qmake, then make."
                                      " You must have Qt 4.5 on your system.");
#endif



                //uiMainWindow.chatText->append("You have the wrong version of the client. Please check http://www.aerolith.org");
                commsSocket->disconnectFromHost();
                return;
            }
            blockSize = packetlength;

        }

        if (commsSocket->bytesAvailable() < blockSize)
            return;

        // ok, we can now read the WHOLE packet
        // ideally we read the 'case' byte right now, and then call a process function with
        // 'in' (the QDataStream) as a parameter!
        // the process function will set blocksize to 0 at the end
        quint8 packetType;
        in >> packetType; // this is the case byte!
        qDebug() << "Packet type " << (char)packetType << "block length" << blockSize;
        switch(packetType)
        {
        case SERVER_PING:
            {
                // keep alive
                writeHeaderData();
                out << (quint8)CLIENT_PONG;
                fixHeaderLength();
                commsSocket->write(block);
            }
            break;
        case SERVER_MAX_BANDWIDTH:
            {
                quint32 maxBandwidth;
                in >> maxBandwidth;
           //     uiMainWindow.progressBarBandwidthUsage->setRange(0, maxBandwidth);
            //    uiMainWindow.progressBarBandwidthUsage->setValue(0);

            }
            break;
        case SERVER_RESET_TODAYS_BANDWIDTH:
            {
              //  uiMainWindow.progressBarBandwidthUsage->setValue(0);
            }
            break;

        case SERVER_LOGGED_IN:	// logged in (entered)
            {
                QString username;
                in >> username;
                QListWidgetItem *it = new QListWidgetItem(username, uiMainWindow.listWidgetPeopleConnected);
                peopleLoggedIn.append(username);
                if (username == currentUsername)
                {
                    // QSound::play("sounds/enter.wav");

                    uiLogin.connectStatusLabel->setText("You have connected!");
                    loginDialog->hide();
                    setWindowTitle(QString(WindowTitle + " - logged in as ") + username);
                    sendClientVersion();
                    gameBoardWidget->setMyUsername(username);
                    currentTablenum = 0;
                }
                if (username.toLower() == "cesar")
                {
                    it->setForeground(QBrush(Qt::blue));

                }
            }
            break;
        case SERVER_LOGGED_OUT:	// logged out
            {
                QString username;
                in >> username;
                for (int i = 0; i < uiMainWindow.listWidgetPeopleConnected->count(); i++)
                    if (uiMainWindow.listWidgetPeopleConnected->item(i)->text() == username)
                    {
                    QListWidgetItem *it = uiMainWindow.listWidgetPeopleConnected->takeItem(i);
                    delete it;
                }
                peopleLoggedIn.removeAll(username);

            }
            break;

        case SERVER_ERROR:	// error
            {
                QString errorString;
                in >> errorString;
                QMessageBox::information(loginDialog, "Aerolith client", errorString);

                uiMainWindow.chatText->append("<font color=red>" + errorString + "</font>");
            }
            break;
        case SERVER_CHAT:	// chat
            {
                QString username;
                in >> username;
                QString text;
                in >> text;
                uiMainWindow.chatText->moveCursor(QTextCursor::End);
                uiMainWindow.chatText->insertHtml(QString("[")+username+"] " + text);
                uiMainWindow.chatText->append("");
            }
            break;
        case SERVER_PM:	// PM
            {
                QString username, message;
                in >> username >> message;
                if (uiMainWindow.checkBoxIgnoreMsgs->isChecked() == false)
                    receivedPM(username, message);
                else
                {
                }// TODO inform user that his message didn't go through
            }
            break;
        case SERVER_NEW_TABLE:	// New table
            {
                // there is a new table

                // static information
                quint16 tablenum;
                quint8 gameType;
                QString lexiconName;
                QString tableName;
                quint8 maxPlayers;
                bool isPrivate;
                //		QStringList

                in >> tablenum >> gameType >> lexiconName >> tableName >> maxPlayers >> isPrivate;
                // create table
                handleCreateTable(tablenum, gameType, lexiconName, tableName, maxPlayers, isPrivate);
                /* TODO genericize this as well (like in the server) to take in a table number and type,
                           then read different amount of info for each type */
            }
            break;
        case SERVER_JOIN_TABLE:	// player joined table
            {
                quint16 tablenum;
                QString playerName;

                in >> tablenum >> playerName;
                qDebug() << playerName << "joined" << tablenum;
                handleAddToTable(tablenum, playerName);
                if (playerName == currentUsername)
                {

                    currentTablenum = tablenum;
                    if (!tables.contains(tablenum))
                        break;
                    tableRepresenter* t = tables.value(tablenum);
                    gameBoardWidget->setTableCapacity(t->maxPlayers);
                    QString wList = t->descriptorItem->text();
                    qDebug() << "I joined table!";
                    gameBoardWidget->resetTable(tablenum, wList, playerName);
                    gameBoardWidget->show();
                    trySitting(0);  // whenever player joins, they try sitting at spot 0.
                    gameBoardWidget->setPrivacy(t->isPrivate);

                    uiMainWindow.comboBoxLexicon->setEnabled(false);

                }
                if (currentTablenum == tablenum)
                    modifyPlayerLists(tablenum, playerName, 1);
                //  chatText->append(QString("%1 has entered %2").arg(playerName).arg(tablenum));


            }
            break;
        case SERVER_TABLE_PRIVACY:
            {
                quint16 tablenum;
                bool privacy;

                in >> tablenum >> privacy;

                if (tables.contains(tablenum))
                {
                    tableRepresenter *t = tables.value(tablenum);
                    t->isPrivate = privacy;
                    t->buttonItem->setEnabled(!t->isPrivate);
                    if (currentTablenum == tablenum)
                        gameBoardWidget->setPrivacy(privacy);
                }
            }
            break;

        case SERVER_INVITE_TO_TABLE:
            {
                quint16 tablenum;
                QString username;
                in >> tablenum >> username;

                if (uiMainWindow.checkBoxIgnoreTableInvites->isChecked() == false)
                {
                    Ui::inviteForm ui;
                    QWidget* inviteWidget = new QWidget(this, Qt::Window);
                    ui.setupUi(inviteWidget);
                    inviteWidget->setAttribute(Qt::WA_DeleteOnClose);
                    inviteWidget->show();

                    ui.pushButtonAccept->setProperty("tablenum", tablenum);
                    ui.pushButtonAccept->setProperty("host", username);

                    ui.pushButtonDecline->setProperty("tablenum", tablenum);
                    ui.pushButtonDecline->setProperty("host", username);

                    connect(ui.pushButtonAccept, SIGNAL(clicked()), SLOT(acceptedInvite()));
                    connect(ui.pushButtonDecline, SIGNAL(clicked()), SLOT(declinedInvite()));
                    ui.labelInfo->setText(QString("You have been invited to table %1 by %2.").arg(tablenum).arg(username));
                    //TODO watch out for abuses of this! (invite bombs?)
                }
                else
                {
                }   // TODO inform user that their invite didn't go through
            }
            break;
        case SERVER_BOOT_FROM_TABLE:
            {
                quint16 tablenum;
                QString username;
                in >> tablenum >> username;
                if (tablenum == currentTablenum)
                    QMessageBox::information(this, "You have been booted",
                                             QString("You have been booted from table %1 by %2").arg(tablenum).arg(username));



            }
            break;
        case SERVER_LEFT_TABLE:
            {
                // player left table
                quint16 tablenum;
                QString playerName;
                in >> tablenum >> playerName;

                if (currentTablenum == tablenum)
                {
                    modifyPlayerLists(tablenum, playerName, -1);
                }

                if (playerName == currentUsername)
                {
                    currentTablenum = 0;
                    //gameStackedWidget->setCurrentIndex(0);
                    gameBoardWidget->hide();
                    uiMainWindow.comboBoxLexicon->setEnabled(true);
                }

                // chatText->append(QString("%1 has left %2").arg(playerName).arg(tablenum));
                handleLeaveTable(tablenum, playerName);

            }

            break;
        case SERVER_KILL_TABLE:
            {
                // table has ceased to exist
                quint16 tablenum;
                in >> tablenum;
                //	    chatText->append(QString("%1 has ceasd to exist").arg(tablenum));
                handleDeleteTable(tablenum);
            }
            break;
        case SERVER_WORD_LISTS:

            // word lists
            handleWordlistsMessage();


            break;
        case SERVER_TABLE_COMMAND:
            // table command
            // an additional byte is needed
            {
                quint16 tablenum;
                in >> tablenum;
                if (tablenum != currentTablenum)
                {
                    qDebug() << "HORRIBLE ERROR" << tablenum << currentTablenum;
                    QMessageBox::critical(this, "Aerolith client", "Critical error 10003");
                    exit(0);
                }
                quint8 addByte;
                in >> addByte;

                handleTableCommand(tablenum, addByte);

            }
            break;
        case SERVER_MESSAGE:
            // server message
            {
                QString serverMessage;
                in >> serverMessage;

                uiMainWindow.chatText->append("<font color=red>" + serverMessage + "</font>");
                gameBoardWidget->gotChat("<font color=red>" + serverMessage + "</font>");

            }
            break;
        case SERVER_HIGH_SCORES:
            // high scores!
            {
                displayHighScores();

            }


            break;

        case SERVER_UNSCRAMBLEGAME_LISTDATA_CLEARALL:
            {
                uiScrambleTable.tableWidgetMyLists->clearContents();
                uiScrambleTable.tableWidgetMyLists->setRowCount(0);
            }
            break;
        case SERVER_UNSCRAMBLEGAME_LISTDATA_ADDONE:
            {
                QString lexicon;
                QStringList labels;
                in >> lexicon >> labels;
                if (lexicon == currentLexicon)
                {
                    uiScrambleTable.tableWidgetMyLists->insertRow(0);
                    for (int j = 0; j < labels.size(); j++)
                        uiScrambleTable.tableWidgetMyLists->setItem(0, j, new QTableWidgetItem(labels[j]));

                }

            }
            break;
        case SERVER_UNSCRAMBLEGAME_LISTDATA_DONE:
            {
                uiScrambleTable.tableWidgetMyLists->resizeColumnsToContents();
            }
            break;
        case SERVER_UNSCRAMBLEGAME_LISTDATA_CLEARONE:
            {
                QString lexicon, listname;
                in >> lexicon >> listname;
                if (lexicon == currentLexicon)
                {
                    for (int i = 0; i < uiScrambleTable.tableWidgetMyLists->rowCount(); i++)
                    {
                        if (uiScrambleTable.tableWidgetMyLists->item(i, 0)->text() == listname)
                        {
                            uiScrambleTable.tableWidgetMyLists->removeRow(i);
                            break;  // break out of for-loop
                        }
                    }
                }
            }
            break;
        case SERVER_UNSCRAMBLEGAME_LISTSPACEUSAGE:
            {
                quint32 usage, max;
                in >> usage >> max;
                uiScrambleTable.progressBarUsedListSpace->setRange(0, max);
                uiScrambleTable.progressBarUsedListSpace->setValue(usage);

            }
            break;
        default:
            QMessageBox::critical(this, "Aerolith client", "Don't understand this packet!");
            commsSocket->disconnectFromHost();
        }

        blockSize = 0;
    }
}


void MainWindow::displayError(QAbstractSocket::SocketError socketError)
{
    switch (socketError)
    {
    case QAbstractSocket::RemoteHostClosedError:
        break;
    case QAbstractSocket::HostNotFoundError:
        uiMainWindow.chatText->append("<font color=red>The host was not found. Please check the "
                                      "host name and port settings.</font>");
        break;
    case QAbstractSocket::ConnectionRefusedError:
        uiMainWindow.chatText->append("<font color=red>The connection was refused by the peer. "
                                      "Make sure the Aerolith server is running, "
                                      "and check that the host name and port "
                                      "settings are correct.</font>");
        break;
    default:
        uiMainWindow.chatText->append(QString("<font color=red>The following error occurred: %1.</font>")
                                      .arg(commsSocket->errorString()));
    }

    uiLogin.loginPushButton->setText("Connect");
    uiLogin.connectStatusLabel->setText("Disconnected.");
}

void MainWindow::toggleConnectToServer()
{
    if (commsSocket->state() != QAbstractSocket::ConnectedState)
    {
        connectionMode = MODE_LOGIN;
        commsSocket->abort();

        commsSocket->connectToHost(uiLogin.serverLE->text(), uiLogin.portLE->text().toInt());
        uiLogin.connectStatusLabel->setText("Connecting to server...");
        uiLogin.loginPushButton->setText("Disconnect");


        uiMainWindow.roomTableWidget->clearContents();
        uiMainWindow.roomTableWidget->setRowCount(0);

        QList <tableRepresenter*> tableStructs = tables.values();
        tables.clear();
        foreach (tableRepresenter* t, tableStructs)
            delete t;
    }
    else
    {
        uiLogin.connectStatusLabel->setText("Disconnected from server");
        commsSocket->disconnectFromHost();
        uiLogin.loginPushButton->setText("Connect");
        //gameStackedWidget->setCurrentIndex(2);
        //centralWidget->hide();
        loginDialog->show();
        loginDialog->raise();
        //gameBoardWidget->hide();
    }

}

void MainWindow::serverDisconnection()
{
    uiLogin.connectStatusLabel->setText("You are disconnected.");
    uiMainWindow.listWidgetPeopleConnected->clear();
    peopleLoggedIn.clear();
    //	QMessageBox::information(this, "Aerolith Client", "You have been disconnected.");
    uiLogin.loginPushButton->setText("Connect");
    //centralWidget->hide();
    loginDialog->show();
    loginDialog->raise();
    uiMainWindow.roomTableWidget->clearContents();
    uiMainWindow.roomTableWidget->setRowCount(0);

    QList <tableRepresenter*> tableStructs = tables.values();
    tables.clear();
    foreach (tableRepresenter* t, tableStructs)
        delete t;


    setWindowTitle("Aerolith - disconnected");
    gameBoardWidget->hide();
    uiMainWindow.comboBoxLexicon->setEnabled(true);
}

void MainWindow::connectedToServer()
{
    // only after connecting!
    blockSize = 0;
    in.setDevice(commsSocket);
    in.setVersion(QDataStream::Qt_4_2);


    // here we see if we are registering a name, or if we are connecting with an existing
    // name/password

    if (connectionMode == MODE_LOGIN)
    {

        currentUsername = uiLogin.usernameLE->text();


        writeHeaderData();
        out << (quint8)CLIENT_LOGIN;
        out << currentUsername;
        out << uiLogin.passwordLE->text();
        fixHeaderLength();
        commsSocket->write(block);
    }
    else if (connectionMode == MODE_REGISTER)
    {
        writeHeaderData();
        out << (quint8)CLIENT_REGISTER;
        out << uiLogin.desiredUsernameLE->text();
        out << uiLogin.desiredFirstPasswordLE->text();
        fixHeaderLength();
        commsSocket->write(block);
    }
}

void MainWindow::sendPM(QString person)
{
    QString hashString = person.toLower();
    if (pmWindows.contains(hashString))
    {
        PMWidget* w = pmWindows.value(hashString);
        w->show();
        w->activateWindow();
        w->raise();
    }
    else
    {
        PMWidget *w = new PMWidget(0, currentUsername, person);
        w->setAttribute(Qt::WA_QuitOnClose, false);
        connect(w, SIGNAL(sendPM(QString, QString)), this, SLOT(sendPM(QString, QString)));
        connect(w, SIGNAL(shouldDelete()), this, SLOT(shouldDeletePMWidget()));
        w->show();

        pmWindows.insert(hashString, w);
    }

}

void MainWindow::sendPM(QString username, QString message)
{
    if (message.simplified() == "")
    {
        return;
    }

    writeHeaderData();
    out << (quint8)CLIENT_PM;
    out << username;
    out << message;
    fixHeaderLength();
    commsSocket->write(block);
}

void MainWindow::receivedPM(QString username, QString message)
{
    QString hashString = username.toLower();
    if (pmWindows.contains(hashString))
    {
        PMWidget* w = pmWindows.value(hashString);
        w->appendText("[" + username + "] " + message);
        w->show();
        w->activateWindow();
        w->raise();

    }
    else
    {
        PMWidget *w = new PMWidget(0, currentUsername, username);
        w->setAttribute(Qt::WA_QuitOnClose, false);
        connect(w, SIGNAL(sendPM(QString, QString)), this, SLOT(sendPM(QString, QString)));
        connect(w, SIGNAL(shouldDelete()), this, SLOT(shouldDeletePMWidget()));
        w->appendText("[" + username + "] " + message);
        w->show();
        pmWindows.insert(hashString, w);
    }
    //QSound::play("sounds/inbound.wav");
}

void MainWindow::shouldDeletePMWidget()
{
    PMWidget* w = static_cast<PMWidget*> (sender());
    pmWindows.remove(w->getRemoteUsername().toLower());
    w->deleteLater();

}

void MainWindow::createUnscrambleGameTable()
{
    if (uiScrambleTable.radioButtonMyLists->isChecked() && uiScrambleTable.tableWidgetMyLists->selectedItems().size() == 0) return;

    writeHeaderData();
    out << (quint8)CLIENT_NEW_TABLE;
    out << (quint8)GAME_TYPE_UNSCRAMBLE;

    quint8 numPlayers = uiScrambleTable.playersSpinBox->value();

    out << numPlayers;


    if (uiScrambleTable.radioButtonOtherLists->isChecked() && uiScrambleTable.listWidgetTopLevelList->currentItem())
    {
        QString listname = uiScrambleTable.listWidgetTopLevelList->currentItem()->text();

        out << (quint8)LIST_TYPE_NAMED_LIST;
        out << listname;

        gameBoardWidget->setUnmodifiedListName(listname);

    }
    else if (uiScrambleTable.radioButtonProbability->isChecked())
    {
        if (!uiScrambleTable.checkBoxAll->isChecked())
        {
            out << (quint8)LIST_TYPE_INDEX_RANGE_BY_WORD_LENGTH;
            if (uiScrambleTable.spinBoxProb2->value() <= uiScrambleTable.spinBoxProb1->value()) return;   // don't send any data, this table is invalid

            quint8 wl;
            quint32 low, high;

            wl = uiScrambleTable.spinBoxWL->value();
            low = uiScrambleTable.spinBoxProb1->value();
            high = uiScrambleTable.spinBoxProb2->value();

            out << wl << low << high;

            gameBoardWidget->setUnmodifiedListName(QString("%1s -- %2 to %3").arg(wl).arg(low).arg(high));

        }
        else
        {
            out << (quint8)LIST_TYPE_ALL_WORD_LENGTH;

            quint8 wl;
            quint32 low, high;

            wl = uiScrambleTable.spinBoxWL->value();

            out << wl;   // special values mean the entire range.

            low = 1;    // one
            high = dbHandler->getNumWordsByLength(currentLexicon, wl);

            gameBoardWidget->setUnmodifiedListName(QString("%1s -- %2 to %3").arg(wl).arg(low).arg(high));

        }
    }
    else if (uiScrambleTable.radioButtonMyLists->isChecked())
    {
        out << (quint8)LIST_TYPE_USER_LIST;
        QList<QTableWidgetItem*> si = uiScrambleTable.tableWidgetMyLists->selectedItems();
        if (si.size() != 5) return;

        quint8 mode;

        if (uiScrambleTable.radioButtonContinueListQuiz->isChecked()) mode = UNSCRAMBLEGAME_USERLIST_MODE_CONTINUE;
        else if (uiScrambleTable.radioButtonRestartListQuiz->isChecked()) mode = UNSCRAMBLEGAME_USERLIST_MODE_RESTART;
        else if (uiScrambleTable.radioButtonQuizFirstMissed->isChecked()) mode = UNSCRAMBLEGAME_USERLIST_MODE_FIRSTMISSED;


        out << mode;
        out << si[0]->text();   // list name -- must match list on server



        gameBoardWidget->setUnmodifiedListName(si[0]->text());
    }




    if (uiScrambleTable.cycleRbo->isChecked()) out << (quint8)TABLE_TYPE_CYCLE_MODE;
    else if (uiScrambleTable.endlessRbo->isChecked()) out << (quint8)TABLE_TYPE_MARATHON_MODE;
    //else if (uiTable.randomRbo->isChecked()) out << (quint8)TABLE_TYPE_RANDOM_MODE;

    out << (quint8)uiScrambleTable.timerSpinBox->value();
    fixHeaderLength();
    commsSocket->write(block);
}

void MainWindow::createBonusGameTable()
{
        writeHeaderData();
        out << (quint8)CLIENT_NEW_TABLE;

        out << (quint8)GAME_TYPE_BONUS;
   //     out << (quint8)uiTable.playersSpinBox->value();
            out << uiMainWindow.comboBoxLexicon->currentText();
    //
    //    out << (quint8)uiMainWindow.comboBoxLexicon->currentIndex();
    //    fixHeaderLength();
    //    commsSocket->write(block);

}

void MainWindow::createNewRoom()
{

    // reset dialog to defaults first.
    uiScrambleTable.cycleRbo->setChecked(true);

    uiScrambleTable.playersSpinBox->setValue(1);
    uiScrambleTable.timerSpinBox->setValue(4);

    createScrambleTableDialog->show();
}

void MainWindow::joinTable()
{
    QPushButton* buttonThatSent = static_cast<QPushButton*> (sender());
    QVariant tn = buttonThatSent->property("tablenum");
    quint16 tablenum = (quint16)tn.toInt();

    writeHeaderData();
    out << (quint8)CLIENT_JOIN_TABLE;
    out << (quint16) tablenum;
    fixHeaderLength();
    commsSocket->write(block);
}

void MainWindow::leaveThisTable()
{
    writeHeaderData();
    out << (quint8)CLIENT_LEAVE_TABLE;
    out << (quint16)currentTablenum;
    fixHeaderLength();
    commsSocket->write(block);
}


void MainWindow::handleTableCommand(quint16 tablenum, quint8 commandByte)
{
    switch (commandByte)
    {
    case SERVER_TABLE_MESSAGE:
        // a message
        {
            QString message;
            in >> message;
            gameBoardWidget->gotChat("<font color=green>" + message + "</font>");


        }
        break;
    case SERVER_TABLE_TIMER_VALUE:
        // a timer byte
        {
            quint16 timerval;
            in >> timerval;
            gameBoardWidget->gotTimerValue(timerval);


        }

        break;
    case SERVER_TABLE_READY_BEGIN:
        // a request for beginning the game from a player
        // refresh ready light for each player.
        {
            quint8 seat;
            in >> seat;
            gameBoardWidget->setReadyIndicator(seat);
        }
        break;
    case SERVER_TABLE_GAME_START:
        // the game has started
        {
            gameBoardWidget->setupForGameStart();
            gameBoardWidget->gotChat("<font color=red>The game has started!</font>");
            gameStarted = true;

            //gameTimer->start(1000);
        }

        break;
    case SERVER_TABLE_GAME_END:
        // the game has ended

        gameBoardWidget->gotChat("<font color=red>This round is over.</font>");
        gameStarted = false;
        gameBoardWidget->populateSolutionsTable();
        ///gameTimer->stop();
        gameBoardWidget->clearAllWordTiles();
        break;


    case SERVER_TABLE_CHAT:
        // chat
        {
            QString username, chat;
            in >> username;
            in >> chat;
            gameBoardWidget->gotChat("[" + username + "] " + chat);
        }
        break;
    case SERVER_TABLE_HOST:
        {
            QString host;
            in >> host;
            gameBoardWidget->setHost(host);
        }
        break;
    case SERVER_TABLE_QUESTIONS:
        // alphagrams!!!
        {
            QTime t;
            t.start();
            quint8 numRacks;
            in >> numRacks;
            for (int i = 0; i < numRacks; i++)
            {
                quint32 probIndex;
                in >> probIndex;
                quint8 numSolutionsNotYetSolved;
                in >> numSolutionsNotYetSolved;
                QSet <quint8> notSolved;

                quint8 temp;
                for (int j = 0; j < numSolutionsNotYetSolved; j++)
                {
                    in >> temp;
                    notSolved.insert(temp);
                }
                gameBoardWidget->addNewWord(i, probIndex, numSolutionsNotYetSolved, notSolved);
            }
            gameBoardWidget->clearSolutionsDialog();

        }
        break;

    case SERVER_TABLE_NUM_QUESTIONS:
        // word list info

        {
            quint16 numRacksSeen;
            quint16 numTotalRacks;
            in >> numRacksSeen >> numTotalRacks;
            gameBoardWidget->gotWordListInfo(QString("%1 / %2").arg(numRacksSeen).arg(numTotalRacks));
            break;
        }

    case SERVER_TABLE_GIVEUP:
        // someone cried uncle
        {
            QString username;
            in >> username;
            gameBoardWidget->gotChat("<font color=red>" + username + " gave up! </font>");
        }
        break;

    case SERVER_TABLE_CORRECT_ANSWER:
        {
            quint8 seatNumber;
            quint8 space, specificAnswer;
            in >> seatNumber >> space >> specificAnswer;
            gameBoardWidget->answeredCorrectly(seatNumber, space, specificAnswer);


        }
        break;
    case SERVER_TABLE_UNSCRAMBLEGAME_MAIN_QUIZ_DONE:
        gameBoardWidget->mainQuizDone();
        break;
    case SERVER_TABLE_UNSCRAMBLEGAME_FULL_QUIZ_DONE:
        gameBoardWidget->fullQuizDone();
        break;
    case SERVER_TABLE_UNSCRAMBLEGAME_SAVING_ALLOWED:
        {
            bool allowed;
            in >> allowed;
            gameBoardWidget->setSavingAllowed(allowed);
        }
        break;
    case SERVER_TABLE_AVATAR_CHANGE:
        // avatar id
        {
            quint8 seatNumber;
            quint8 avatarID;
            in >> seatNumber >> avatarID;

            gameBoardWidget->setAvatar(seatNumber, avatarID);

            // then here we can do something like chatwidget->setavatar( etc). but this requires the server
            // to send avatars to more than just the table. so if we want to do this, we need to change the server behavior!
            // this way we can just send everyone's avatar on login. consider changing this!
        }
        break;

    case SERVER_TABLE_SUCCESSFUL_STAND:
        {
            QString username;
            quint8 seatNumber;

            in >> username >> seatNumber;
            gameBoardWidget->standup(username, seatNumber);
        }
        break;

    case SERVER_TABLE_SUCCESSFUL_SIT:
        {
            QString username;
            quint8 seatNumber;
            in >> username >> seatNumber;
            gameBoardWidget->sitdown(username, seatNumber);
            qDebug() << "Got saet packet" << username << seatNumber;

        }
        break;



    }


}

void MainWindow::handleWordlistsMessage()
{

    quint8 numLexica;
    in >> numLexica;
    disconnect(uiMainWindow.comboBoxLexicon, SIGNAL(currentIndexChanged(QString)), 0, 0);
    uiMainWindow.comboBoxLexicon->clear();
    lexiconListsHash.clear();
    qDebug() << "Got" << numLexica << "lexica.";

    QHash <int, QString> localLexHash;

    for (int i = 0; i < numLexica; i++)
    {
        QByteArray lexicon;
        in >> lexicon;
        localLexHash.insert(i, QString(lexicon));

        if (existingLocalDBList.contains(lexicon))
            uiMainWindow.comboBoxLexicon->addItem(lexicon);
        LexiconLists dummyLists;
        dummyLists.lexicon = lexicon;
        lexiconListsHash.insert(QString(lexicon), dummyLists);

    }


    quint8 numTypes;
    in >> numTypes;
    for (int i = 0; i < numTypes; i++)
    {
        quint8 type;
        in >> type;
        switch(type)
        {

        case 'R':
            {
                for (int j = 0; j < numLexica; j++)
                {
                    quint8 lexiconIndex;
                    quint16 numLists;
                    in >> lexiconIndex >> numLists;

                    QString lexicon = localLexHash.value(lexiconIndex);
                    for (int k = 0; k < numLists; k++)
                    {
                        QByteArray listTitle;
                        in >> listTitle;
                        lexiconListsHash[lexicon].regularWordLists << listTitle;
                    }
                }
            }
            break;


        case 'D':
            {
                for (int j = 0; j < numLexica; j++)
                {
                    quint8 lexiconIndex;
                    quint16 numLists;
                    in >> lexiconIndex >> numLists;

                    QString lexicon = localLexHash.value(lexiconIndex);

                    for (int k = 0; k < numLists; k++)
                    {
                        QByteArray listTitle;
                        in >> listTitle;
                        lexiconListsHash[lexicon].dailyWordLists << listTitle;
                    }
                }
            }
            break;

        }
    }

    if (uiMainWindow.comboBoxLexicon->count() > 0)
    {
        uiMainWindow.comboBoxLexicon->setCurrentIndex(0);
        lexiconComboBoxIndexChanged(uiMainWindow.comboBoxLexicon->currentText());
    }
    else
    {
        QMessageBox::critical(this, "No Lexicon Databases!", "You have no lexicon databases built. You will not be"
                              " able to play Aerolith without building a lexicon database. Please select the 'Lexica'"
                              " option from the menu and build at least one lexicon database, then reconnect to Aerolith.");


    }
    /* we connect the signals here instead of earlier in the constructor for some reason having to do with the
       above two lines. the 'disconnect' is earlier in this function */


    connect(uiMainWindow.comboBoxLexicon, SIGNAL(currentIndexChanged(QString)),
            SLOT(lexiconComboBoxIndexChanged(QString)));

    spinBoxWordLengthChange(uiScrambleTable.spinBoxWL->value());


}

void MainWindow::lexiconComboBoxIndexChanged(QString lex)
{
    uiScrambleTable.listWidgetTopLevelList->clear();
    challengesMenu->clear();
    uiScores.comboBoxChallenges->clear();

    for (int i = 0; i < lexiconListsHash[lex].regularWordLists.size(); i++)
    {
        uiScrambleTable.listWidgetTopLevelList->addItem(lexiconListsHash[lex].regularWordLists.at(i));
    }
    for (int i = 0; i < lexiconListsHash[lex].dailyWordLists.size(); i++)
    {
        challengesMenu->addAction(lexiconListsHash[lex].dailyWordLists.at(i));
        uiScores.comboBoxChallenges->addItem(lexiconListsHash[lex].dailyWordLists.at(i));
    }
    challengesMenu->addAction("Get today's scores");
    // gameBoardWidget->setDatabase(lexiconLists.at(index).lexicon);
    gameBoardWidget->setLexicon(lex);
    currentLexicon = lex;
    uiScrambleTable.labelLexiconName->setText(currentLexicon);
    wordFilter->setCurrentLexicon(currentLexicon);
    repopulateMyListsTable();

}


void MainWindow::handleCreateTable(quint16 tablenum, quint8 gameType, QString lexiconName,
                                   QString tableName, quint8 maxPlayers, bool isPrivate)
{
    tableRepresenter* t = new tableRepresenter;
    t->tableNumItem = new QTableWidgetItem(QString("%1").arg(tablenum));
    t->descriptorItem = new QTableWidgetItem("(" + lexiconName +
                                             ") " + tableName);
    t->playersItem = new QTableWidgetItem("");
    t->maxPlayers = maxPlayers;
    t->isPrivate = isPrivate;
    switch (gameType)
    {
    case GAME_TYPE_UNSCRAMBLE:

        t->typeIcon = new QTableWidgetItem(unscrambleGameIcon, "");
        t->typeIcon->setSizeHint(QSize(32, 32));
        break;
    case GAME_TYPE_BONUS:
        t->typeIcon = new QTableWidgetItem(QIcon(":/images/BonusGameSmall.png"), "");
        t->typeIcon->setSizeHint(QSize(32, 32));
        break;
    }
    t->buttonItem = new QPushButton("Join");
    t->buttonItem->setProperty("tablenum", QVariant((quint16)tablenum));
    t->tableNum = tablenum;
    connect(t->buttonItem, SIGNAL(clicked()), this, SLOT(joinTable()));
    t->buttonItem->setEnabled(!t->isPrivate);

    //  QStringList playerList;
    //t->playersItem->setData(PLAYERLIST_ROLE, QVariant(playerList));

    int rc = uiMainWindow.roomTableWidget->rowCount();

    uiMainWindow.roomTableWidget->insertRow(rc);
    uiMainWindow.roomTableWidget->setItem(rc, 0, t->tableNumItem);
    uiMainWindow.roomTableWidget->setItem(rc, 1, t->typeIcon);
    uiMainWindow.roomTableWidget->setItem(rc, 2, t->descriptorItem);
    uiMainWindow.roomTableWidget->setItem(rc, 3, t->playersItem);
    uiMainWindow.roomTableWidget->setCellWidget(rc, 4, t->buttonItem);
    uiMainWindow.roomTableWidget->setRowHeight(rc, 40);

    //   uiMainWindow.roomTableWidget->resizeColumnsToContents();
    tables.insert(tablenum, t);



}

void MainWindow::modifyPlayerLists(quint16 tablenum, QString player, int modification)
{
    // modifies the player lists INSIDE a table

    if (!tables.contains(tablenum))
        return;
    tableRepresenter* t = tables.value(tablenum);
    if (player == currentUsername)	// I joined. therefore, populate the list from the beginning
    {
        if (modification == -1)
        {
            gameBoardWidget->leaveTable();

            return; // the widget will be hid anyway, so we don't need to hide the individual lists
            //however, we hide when adding when we join down below
        }
        else
        {
            gameBoardWidget->addPlayers(t->playerList);
            // add all players including self
            return;
        }
    }

    // if we are here then a player has joined/left a table that we were already in

    // modification = -1 remove
    // or 1 add

    if (modification == 1)
        // player has been added
        // find a spot
        gameBoardWidget->addPlayer(player, gameStarted);

    else if (modification == -1)
        gameBoardWidget->removePlayer(player, gameStarted);


}



void MainWindow::handleDeleteTable(quint16 tablenum)
{

    if (!tables.contains(tablenum))
        return;

    tableRepresenter* t = tables.value(tablenum);
    uiMainWindow.roomTableWidget->removeRow(t->tableNumItem->row()); // delete the whole row (this function deletes the elements)
    delete tables.take(tablenum); // delete the tableRepresenter object after removing it from the hash.
}

void MainWindow::handleLeaveTable(quint16 tablenum, QString player)
{


    if (!tables.contains(tablenum))
        return;

    tableRepresenter* t = tables.value(tablenum);

    t->playerList.removeAll(player);
    QString textToSet="";
    foreach(QString thisplayer, t->playerList)
    {
        textToSet += thisplayer + " ";
    }

    int numPlayers = t->playerList.size();
    textToSet += "(" + QString::number(numPlayers) + "/" + QString::number(t->maxPlayers) + ")";
    t->playersItem->setText(textToSet);

}

void MainWindow::handleAddToTable(quint16 tablenum, QString player)
{
    // this will change button state as well

    if (!tables.contains(tablenum))
        return;

    tableRepresenter* t = tables.value(tablenum);
    t->playerList << player;
    QString textToSet = "";
    foreach(QString thisplayer, t->playerList)
    {
        textToSet += thisplayer + " ";

    }

    quint8 numPlayers = t->playerList.size();
    quint8 maxPlayers = t->maxPlayers;

    textToSet += "(" + QString::number(numPlayers) + "/" + QString::number(maxPlayers) + ")";

    t->playersItem->setText(textToSet);

}


void MainWindow::giveUpOnThisGame()
{
    writeHeaderData();
    out << (quint8)CLIENT_TABLE_COMMAND;
    out << (quint16)currentTablenum;
    out << (quint8)CLIENT_TABLE_GIVEUP;
    fixHeaderLength();
    commsSocket->write(block);
}

void MainWindow::trySitting(quint8 seatNumber)
{
    writeHeaderData();
    out << (quint8)CLIENT_TABLE_COMMAND;
    out << (quint16)currentTablenum;
    out << (quint8)CLIENT_TRY_SITTING;
    out << seatNumber;
    fixHeaderLength();
    commsSocket->write(block);
}

void MainWindow::standUp()
{
    writeHeaderData();
    out << (quint8)CLIENT_TABLE_COMMAND;
    out << (quint16)currentTablenum;
    out << (quint8)CLIENT_TRY_STANDING;

    fixHeaderLength();
    commsSocket->write(block);

}

void MainWindow::trySetTablePrivate(bool priv)
{
    writeHeaderData();
    out << (quint8)CLIENT_TABLE_COMMAND;
    out << (quint16)currentTablenum;
    out << (quint8)CLIENT_TABLE_PRIVACY;
    out << priv;
    fixHeaderLength();
    commsSocket->write(block);
}

void MainWindow::submitReady()
{
    writeHeaderData();
    out << (quint8)CLIENT_TABLE_COMMAND;
    out << (quint16)currentTablenum;
    out << (quint8)CLIENT_TABLE_READY_BEGIN;
    fixHeaderLength();
    commsSocket->write(block);

}

void MainWindow::aerolithHelpDialog()
{
    QString infoText;
    infoText += "- Cycle mode allows you to go through all the words in a list, and at the end, keep going through the missed words.<BR>";
    infoText += "- Tables are very customizable. To customize the colors and tiles in a table, please enter a table, and click the ""Preferences"" button. It may help to click start to see what the tiles will look like. Click Save when you are done.<BR>";
    QMessageBox::information(this, "Aerolith how-to", infoText);
}
void MainWindow::aerolithAcknowledgementsDialog()
{
    QFile file(":acknowledgments.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    QString infoText = file.readAll();
    QMessageBox::information(this, "Acknowledgements", infoText);
    file.close();
}

void MainWindow::showAboutQt()
{
    QMessageBox::aboutQt(this, "About Qt");
}

void MainWindow::displayHighScores()
{
    QString challengeName;
    quint16 numSolutions;
    quint16 numEntries;
    in >> challengeName >> numSolutions >> numEntries;
    QString username;
    quint16 numCorrect;
    quint16 timeRemaining;
    uiScores.scoresTableWidget->clearContents();
    uiScores.scoresTableWidget->setRowCount(0);


    QList <tempHighScoresStruct> temp;
    for (int i = 0; i < numEntries; i++)
    {
        in >> username >> numCorrect >> timeRemaining;
        temp << tempHighScoresStruct(username, numCorrect, timeRemaining);
    }
    qSort(temp.begin(), temp.end(), highScoresLessThan);

    for (int i = 0; i < numEntries; i++)
    {

        QTableWidgetItem* rankItem = new QTableWidgetItem(QString("%1").arg(i+1));
        QTableWidgetItem* usernameItem = new QTableWidgetItem(temp.at(i).username);
        QTableWidgetItem* correctItem = new QTableWidgetItem(QString("%1%").arg(100.0*(double)temp.at(i).numCorrect/(double)numSolutions, 0, 'f', 1));
        QTableWidgetItem* timeItem = new QTableWidgetItem(QString("%1 s").arg(temp.at(i).timeRemaining));
        uiScores.scoresTableWidget->insertRow(uiScores.scoresTableWidget->rowCount());
        uiScores.scoresTableWidget->setItem(uiScores.scoresTableWidget->rowCount() -1, 0, rankItem);
        uiScores.scoresTableWidget->setItem(uiScores.scoresTableWidget->rowCount() -1, 1, usernameItem);
        uiScores.scoresTableWidget->setItem(uiScores.scoresTableWidget->rowCount() -1, 2, correctItem);
        uiScores.scoresTableWidget->setItem(uiScores.scoresTableWidget->rowCount() -1, 3, timeItem);
    }

    uiScores.scoresTableWidget->resizeColumnsToContents();
}


void MainWindow::sendClientVersion()
{
    writeHeaderData();
    out << (quint8)CLIENT_VERSION;
    out << aerolithVersion;
    fixHeaderLength();
    commsSocket->write(block);
}



void MainWindow::updateGameTimer()
{
    //if (gameBoardWidget->timerDial->value() == 0) return;

    //gameBoardWidget->timerDial->display(gameBoardWidget->timerDial->value() - 1);

}

void MainWindow::dailyChallengeSelected(QAction* challengeAction)
{
    if (challengeAction->text() == "Get today's scores")
    {
        uiScores.scoresTableWidget->clearContents();
        uiScores.scoresTableWidget->setRowCount(0);
        scoresDialog->show();
    }
    else
    {
        writeHeaderData();
        out << (quint8)CLIENT_NEW_TABLE;
        out << (quint8)GAME_TYPE_UNSCRAMBLE;
        out << (quint8)1; // 1 player
        out << (quint8)LIST_TYPE_DAILY_CHALLENGE;
        out << challengeAction->text();
        out << uiMainWindow.comboBoxLexicon->currentText(); // TODO this is kind of kludgy, should already know what lexicon
        // I'm on.
        out << (quint8)TABLE_TYPE_DAILY_CHALLENGES;
        out << (quint8)0;	// server should decide time for daily challenge

        fixHeaderLength();
        commsSocket->write(block);
    }
}

void MainWindow::getScores()
{
    uiScores.scoresTableWidget->clearContents();
    uiScores.scoresTableWidget->setRowCount(0);
    writeHeaderData();
    out << (quint8)CLIENT_HIGH_SCORE_REQUEST;
    out << uiScores.comboBoxChallenges->currentText();
    fixHeaderLength();
    commsSocket->write(block);


}

void MainWindow::registerName()
{

    //  int retval = registerDialog->exec();
    //if (retval == QDialog::Rejected) return;
    // validate password
    if (uiLogin.desiredFirstPasswordLE->text() != uiLogin.desiredSecondPasswordLE->text())
    {
        QMessageBox::warning(this, "Aerolith", "Your passwords do not match! Please try again");
        uiLogin.desiredFirstPasswordLE->clear();
        uiLogin.desiredSecondPasswordLE->clear();
        return;

    }



    //

    connectionMode = MODE_REGISTER;
    commsSocket->abort();
    commsSocket->connectToHost(uiLogin.serverLE->text(), uiLogin.portLE->text().toInt());
    uiLogin.connectStatusLabel->setText("Connecting to server...");
    uiLogin.loginPushButton->setText("Disconnect");
}

void MainWindow::showRegisterPage()
{
    uiLogin.stackedWidget->setCurrentIndex(1);
}

void MainWindow::showLoginPage()
{
    uiLogin.stackedWidget->setCurrentIndex(0);
}

void MainWindow::viewProfile(QString username)
{
    uiGetProfile.lineEditUsername->setText(username);
    getProfileWidget->show();

}

void MainWindow::spinBoxWordLengthChange(int length)
{
    int max = dbHandler->getNumWordsByLength(currentLexicon, length);
    if (max != 0)
        uiScrambleTable.spinBoxProb2->setMaximum(max);
}

void MainWindow::startOwnServer()
{
    // start a server thread
    if (uiLogin.pushButtonStartOwnServer->text() == "Start Own Server")
    {
        emit startServerThread();
        uiLogin.loginPushButton->setEnabled(false);
        uiLogin.pushButtonStartOwnServer->setEnabled(false);
        uiLogin.connectStatusLabel->setText("Please wait a few seconds while Aerolith Server loads.");
    }
    else
    {

        emit stopServerThread();
    }

}

void MainWindow::serverThreadHasStarted()
{
    uiLogin.pushButtonStartOwnServer->setEnabled(true);
    uiLogin.loginPushButton->setEnabled(true);
    uiLogin.connectStatusLabel->setText("You can now play locally! Log in now.");
    uiLogin.pushButtonStartOwnServer->setText("Stop Server");
    uiLogin.serverLE->setText("localhost");
    uiLogin.portLE->setText(QString::number(DEFAULT_PORT));
}

void MainWindow::serverThreadHasFinished()
{

    uiLogin.connectStatusLabel->setText("Local server has stopped!");
    uiLogin.pushButtonStartOwnServer->setText("Start Own Server");

    uiLogin.serverLE->setText("aerolith.org");
    uiLogin.portLE->setText(QString::number(DEFAULT_PORT));
    uiLogin.loginPushButton->setEnabled(true);

}

void MainWindow::on_pushButtonImportList_clicked()
{
    QString listName = QInputDialog::getText(this, "List name", "Please enter a name for this list");
    if (listName == "") return;
    QString filename = QFileDialog::getOpenFileName(this, "Select a list of words or alphagrams");

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream in(&file);
    QStringList words;
    while (!in.atEnd())
    {
        QString line = in.readLine().simplified();
        if (line == line.section(" ", 0, 0))
        {
            /* basically, just ensuring that the line just has the word */
            words << line;

        }
    }

    QList <quint32> probIndices;

    bool success = dbHandler->getProbIndices(words, currentLexicon, probIndices);

    if (!success)
    {
        QMessageBox::warning(this, "Error", "You must first create a database for this lexicon!");
        return;
    }

    if (probIndices.size() > REMOTE_LISTSIZE_LIMIT)
    {
        QMessageBox::warning(this, "Error", "This list contains more than " + QString::number(REMOTE_LISTSIZE_LIMIT)
                             + " alphagrams. If you would like to upload "
                             "a very large list please do so locally.");
        return;

    }
    // split up list into chunks and send one at a time.
    qDebug() << "probindices" << probIndices;
    writeHeaderData();
    out << (quint8)CLIENT_UNSCRAMBLEGAME_LIST_UPLOAD;
    out << (quint8)2;   // this means what follows is the size of the list
    out << (quint32)probIndices.size();
    out << currentLexicon;
    out << listName.mid(0, 64);
    fixHeaderLength();
    commsSocket->write(block);

    do
    {

        writeHeaderData();
        out << (quint8)CLIENT_UNSCRAMBLEGAME_LIST_UPLOAD;
        out << (quint8)1;   // this means that this list is CONTINUED (i.e. tell the server to wait for more of these packets)
        out << probIndices.mid(0, 2000);
        fixHeaderLength();
        commsSocket->write(block);

        probIndices = probIndices.mid(2000);
    } while (probIndices.size() > 2000);

    writeHeaderData();
    out << (quint8)CLIENT_UNSCRAMBLEGAME_LIST_UPLOAD;
    out << (quint8)0;   // this means that this list is DONE
    out << probIndices;
    fixHeaderLength();
    commsSocket->write(block);

    //    success = dbHandler->saveNewLists(currentLexicon, listName, probIndices);
    //    if (!success)
    //    {
    //        QMessageBox::critical(this, "Error", "Was unable to connect to userlists database! Please inform "
    //                              "delsolar@gmail.com about this.");
    //        return;
    //    }

}

void MainWindow::repopulateMyListsTable()
{
    writeHeaderData();
    out << (quint8)CLIENT_UNSCRAMBLEGAME_LISTINFO_REQUEST;
    out << currentLexicon;
    fixHeaderLength();

    commsSocket->write(block);


}

void MainWindow::showInviteDialog()
{
    QStringList playerList = peopleLoggedIn;
    playerList.removeAll(currentUsername);

    if (playerList.size() >= 1)
    {
        QString playerToInvite = QInputDialog::getItem(this, "Invite", "Select Player to Invite", playerList, 0, false);

        if (playerToInvite != "")
        {

            writeHeaderData();
            out << (quint8)CLIENT_TABLE_COMMAND;
            out << (quint16)currentTablenum;
            out << (quint8)CLIENT_TABLE_INVITE;
            out << playerToInvite;
            fixHeaderLength();
            commsSocket->write(block);
        }
    }

}

void MainWindow::bootFromTable(QString playerToBoot)
{
    writeHeaderData();
    out << (quint8)CLIENT_TABLE_COMMAND;
    out << (quint16)currentTablenum;
    out << (quint8)CLIENT_TABLE_BOOT;
    out << playerToBoot;
    fixHeaderLength();
    commsSocket->write(block);
}

void MainWindow::saveGame()
{
    writeHeaderData();
    out << (quint8)CLIENT_TABLE_COMMAND;
    out << (quint16)currentTablenum;
    out << (quint8)CLIENT_TABLE_UNSCRAMBLEGAME_SAVE_REQUEST;
    fixHeaderLength();
    commsSocket->write(block);

}

void MainWindow::on_radioButtonProbability_clicked()
{
    uiScrambleTable.stackedWidget->setCurrentIndex(0);
}

void MainWindow::on_radioButtonOtherLists_clicked()
{
    uiScrambleTable.stackedWidget->setCurrentIndex(1);
}

void MainWindow::on_radioButtonMyLists_clicked()
{
    uiScrambleTable.stackedWidget->setCurrentIndex(2);
    uiScrambleTable.radioButtonContinueListQuiz->setChecked(true);

}

void MainWindow::on_pushButtonDeleteList_clicked()
{
    QList<QTableWidgetItem*> si = uiScrambleTable.tableWidgetMyLists->selectedItems();
    if (si.size() != 5) return;

    if (QMessageBox::Yes == QMessageBox::warning(this, "Are you sure?", "Are you sure you wish to delete '" +
                                                 si[0]->text() + "'?",
                                                 QMessageBox::Yes | QMessageBox::No, QMessageBox::No))
    {


        writeHeaderData();
        out << (quint8)CLIENT_UNSCRAMBLEGAME_DELETE_LIST;
        out << currentLexicon;
        out << si[0]->text();
        fixHeaderLength();
        commsSocket->write(block);



    }

}

void MainWindow::acceptedInvite()
{
    QPushButton* p = static_cast<QPushButton*> (sender());
    quint16 tablenum = p->property("tablenum").toInt();

    writeHeaderData();
    out << (quint8)CLIENT_JOIN_TABLE;
    out << (quint16) tablenum;
    fixHeaderLength();
    commsSocket->write(block);

    p->parent()->deleteLater();
}

void MainWindow::declinedInvite()
{
    QPushButton* p = static_cast<QPushButton*> (sender());




    p->parent()->deleteLater();
}

void MainWindow::socketWroteBytes(qint64 num)
{
  //  uiMainWindow.progressBarBandwidthUsage->setValue(uiMainWindow.progressBarBandwidthUsage->value() + num);
}

void MainWindow::showDonationPage()
{

    QString donationUrl = "https://www.paypal.com/us/cgi-bin/webscr?cmd=_flow&SESSION=HAV_kL7GVkuKQpwAI-"
                          "OnlVT3eB8uvWuz4wOzWMlMSSfn7NwCnQjZ-Uko92u&dispatch=50a222a57771920b6a3d7b60623"
                          "9e4d529b525e0b7e69bf0224adecfb0124e9b5efedb82468478c6e115945fd0658595b0be0417af"
                          "d2208f";

    QDesktopServices::openUrl(QUrl(donationUrl));
}

void MainWindow::on_actionSubmitSuggestion_triggered()
{
    if (commsSocket->state() != QAbstractSocket::ConnectedState ||
        uiLogin.serverLE->text() == "localhost")
    {
        QMessageBox::warning(this, "Log in", "Please log in to the main Aerolith server and then try "
                             "again.");
        return;
    }
    bool ok;
 //   QMessageBox::warning(this, "woo", "a suggestion to submit!");
    QString text =  QInputDialog::getText(this, "Suggestion/Bug report",
                                          "Enter your suggestion or bug report. Please "
                                          "be detailed and try to remember what triggered a bug.",
                                          QLineEdit::Normal,
                                          "",
                                          &ok);


    if (ok)
    {
        writeHeaderData();
        out << (quint8)CLIENT_SUGGESTION_OR_BUG_REPORT;
        out << text.left(1000);
        fixHeaderLength();
        commsSocket->write(block);

        QMessageBox::information(this, "Thank you", "Thank you for your input!");
    }
}

void MainWindow::on_comboBoxGameType_currentIndexChanged(QString text)
{
    if (text == "WordScramble")
    {
        uiMainWindow.pushButtonChallenges->setVisible(true);
    }
    else
        uiMainWindow.pushButtonChallenges->setVisible(false);

}

/////////////////////////////////////////////////////

PMWidget::PMWidget(QWidget* parent, QString localUsername, QString remoteUsername) :
        QWidget(parent), localUsername(localUsername), remoteUsername(remoteUsername)
{

    uiPm.setupUi(this);
    connect(uiPm.lineEdit, SIGNAL(returnPressed()), this, SLOT(readAndSendLEContents()));
    setWindowTitle("Conversation with " + remoteUsername);
}

void PMWidget::readAndSendLEContents()
{
    emit sendPM(remoteUsername, uiPm.lineEdit->text());
    uiPm.textEdit->append("[" + localUsername + "] " + uiPm.lineEdit->text());

    uiPm.lineEdit->clear();
    QSound::play("sounds/outbound.wav");
}

void PMWidget::appendText(QString text)
{
    uiPm.textEdit->append(text);
}

void PMWidget::closeEvent(QCloseEvent* event)
{
    event->accept();
    emit shouldDelete();
}

////////////////////
/* debug */
void MainWindow::testFunction()
{
    QVector <int> a;
    a << 1 << 2 << 3;
    qDebug () << a.mid(4);
}
