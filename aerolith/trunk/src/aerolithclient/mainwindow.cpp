#include "mainwindow.h"
#include "commonDefs.h"

const quint16 MAGIC_NUMBER = 25348;
const QString WindowTitle = "Aerolith 0.4.1";
//const QString thisVersion = "0.4.1";

bool highScoresLessThan(const tempHighScoresStruct& a, const tempHighScoresStruct& b)
{
    if (a.numCorrect == b.numCorrect) return (a.timeRemaining > b.timeRemaining);
    else return (a.numCorrect > b.numCorrect);
}

MainWindow::MainWindow(QString aerolithVersion) : aerolithVersion(aerolithVersion), PLAYERLIST_ROLE(Qt::UserRole), 
out(&block, QIODevice::WriteOnly)
{




    uiMainWindow.setupUi(this);
    uiMainWindow.roomTableWidget->verticalHeader()->hide();
    uiMainWindow.roomTableWidget->setColumnWidth(0, 30);
    uiMainWindow.roomTableWidget->setColumnWidth(1, 200);
    uiMainWindow.roomTableWidget->setColumnWidth(2, 300);
    uiMainWindow.roomTableWidget->setColumnWidth(3, 50);
    uiMainWindow.roomTableWidget->setColumnWidth(4, 50);
    uiMainWindow.roomTableWidget->setColumnWidth(5, 75);
    uiMainWindow.roomTableWidget->horizontalHeader()->setResizeMode(QHeaderView::Fixed);

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

    commsSocket = new QTcpSocket;
    connect(commsSocket, SIGNAL(readyRead()), this, SLOT(readFromServer()));
    connect(commsSocket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(displayError(QAbstractSocket::SocketError)));
    connect(commsSocket, SIGNAL(connected()), this, SLOT(connectedToServer()));

    connect(commsSocket, SIGNAL(disconnected()), this, SLOT(serverDisconnection()));

    createTableDialog = new QDialog(this);
    uiTable.setupUi(createTableDialog);

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


    gameBoardWidget = new UnscrambleGameTable(0, Qt::Window);
    gameBoardWidget->setWindowTitle("Table");
    gameBoardWidget->setWindowFlags(Qt::WindowMinMaxButtonsHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint);
    gameBoardWidget->setAttribute(Qt::WA_QuitOnClose, false);

    connect(gameBoardWidget, SIGNAL(giveUp()), this, SLOT(giveUpOnThisGame()));
    connect(gameBoardWidget, SIGNAL(sendStartRequest()), this, SLOT(submitReady()));
    connect(gameBoardWidget, SIGNAL(avatarChange(quint8)), this, SLOT(changeMyAvatar(quint8)));
    connect(gameBoardWidget, SIGNAL(guessSubmitted(QString)), this, SLOT(submitGuess(QString)));
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


    gameTimer = new QTimer();
    connect(gameTimer, SIGNAL(timeout()), this, SLOT(updateGameTimer()));

    readWindowSettings();

    show();
    loginDialog->show();
    //  loginDialog->activateWindow();
    loginDialog->raise();
    uiLogin.usernameLE->setFocus(Qt::OtherFocusReason);
    setWindowIcon(QIcon(":images/aerolith.png"));


    setProfileWidget = new QWidget;
    uiSetProfile.setupUi(setProfileWidget);

    getProfileWidget = new QWidget;
    uiGetProfile.setupUi(getProfileWidget);

    ///////


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

void MainWindow::submitGuess(QString guess)
{
    // chatText->append(QString("From solution: ") + solutionLE->text());
    // solutionLE->clear();
    if (guess.length() > 15) return;

    writeHeaderData();
    out << (quint8) CLIENT_TABLE_COMMAND;
    out << (quint16) currentTablenum;
    out << (quint8) CLIENT_TABLE_GUESS; // from solution box
    out << guess;
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
                if (QMessageBox::critical(this, "Wrong version of Aerolith", "You seem to be using an outdated "
                                          "version of Aerolith. If you would like to update, please click OK"
                                          " below.", QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Ok)
                {
#ifdef Q_WS_MAC

                    QDesktopServices::openUrl(QUrl("http://www.aerolith.org"));
                    QCoreApplication::quit();
#endif
#ifdef Q_WS_WIN
                    // call an updater program
                    QDesktopServices::openUrl(QUrl("http://www.aerolith.org"));
                    QCoreApplication::quit();
#endif
                }


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

                case SERVER_LOGGED_IN:	// logged in (entered)
                    {
                        QString username;
                        in >> username;
                        QListWidgetItem *it = new QListWidgetItem(username, uiMainWindow.listWidgetPeopleConnected);
                        if (username == currentUsername)
                        {
                            QSound::play("sounds/enter.wav");

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

                    }
                    break;

                case SERVER_ERROR:	// error
                    {
                        QString errorString;
                        in >> errorString;
                        QMessageBox::information(loginDialog, "Aerolith client", errorString);
                        //	chatText->append("<font color=red>" + errorString + "</font>");
                    }
                    break;
                case SERVER_CHAT:	// chat
                    {
                        QString username;
                        in >> username;
                        QString text;
                        in >> text;
                        uiMainWindow.chatText->append(QString("[")+username+"] " + text);
                    }
                    break;
                case SERVER_PM:	// PM
                    {
                        QString username, message;
                        in >> username >> message;
                        receivedPM(username, message);
                    }
                    break;
                case SERVER_NEW_TABLE:	// New table
                    {
                        // there is a new table

                        // static information
                        quint16 tablenum;
                        QString wordListDescriptor;
                        quint8 maxPlayers;
                        //		QStringList

                        in >> tablenum >> wordListDescriptor >> maxPlayers;
                        // create table
                        handleCreateTable(tablenum, wordListDescriptor, maxPlayers);
                    }
                    break;
                case SERVER_JOIN_TABLE:	// player joined table
                    {
                        quint16 tablenum;
                        QString playerName;

                        in >> tablenum >> playerName; // this will also black out the corresponding button for can join
                        handleAddToTable(tablenum, playerName);
                        if (playerName == currentUsername)
                        {
                            currentTablenum = tablenum;
                            if (!tables.contains(tablenum))
                                break;
                            tableRepresenter* t = tables.value(tablenum);

                            QString wList = t->descriptorItem->text();

                            gameBoardWidget->resetTable(tablenum, wList, playerName);
                            gameBoardWidget->show();

                        }
                        if (currentTablenum == tablenum)
                            modifyPlayerLists(tablenum, playerName, 1);
                        //  chatText->append(QString("%1 has entered %2").arg(playerName).arg(tablenum));


                    }
                    break;
                case SERVER_LEFT_TABLE:
                    {
                        // player left table
                        quint16 tablenum;
                        QString playerName;
                        in >> tablenum >> playerName;

                        if (currentTablenum == tablenum)
                        {//i love shoe
                            modifyPlayerLists(tablenum, playerName, -1);
                        }

                        if (playerName == currentUsername)
                        {
                            currentTablenum = 0;
                            //gameStackedWidget->setCurrentIndex(0);
                            gameBoardWidget->hide();
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
                case SERVER_AVATAR_CHANGE:
                    // avatar id
                    {
                        QString username;
                        quint8 avatarID;
                        in >> username >> avatarID;
                        // username changed his avatar to avatarID. if we want to display this avatar, display it
                        // i.e. if we are in a table. in the future consider changing this to just a table packet but do the check now
                        // just in case.
                        if (currentTablenum != 0)
                        {
                            // we are in a table
                            gameBoardWidget->setAvatar(username, avatarID);

                        }
                        // then here we can do something like chatwidget->setavatar( etc). but this requires the server
                        // to send avatars to more than just the table. so if we want to do this, we need to change the server behavior!
                        // this way we can just send everyone's avatar on login. consider changing this!
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
    }
    else
    {
        PMWidget *w = new PMWidget(0, currentUsername, person);
        w->setAttribute(Qt::WA_QuitOnClose, false);
        connect(w, SIGNAL(sendPM(QString, QString)), this, SLOT(sendPM(QString, QString)));
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
        //w->activateWindow();
    }
    else
    {
        PMWidget *w = new PMWidget(0, currentUsername, username);
        w->setAttribute(Qt::WA_QuitOnClose, false);
        connect(w, SIGNAL(sendPM(QString, QString)), this, SLOT(sendPM(QString, QString)));
        w->appendText("[" + username + "] " + message);
        w->show();
        pmWindows.insert(hashString, w);
    }
    QSound::play("sounds/inbound.wav");

}

void MainWindow::createNewRoom()
{

    // reset dialog to defaults first.
    uiTable.cycleRbo->setChecked(true);

    uiTable.playersSpinBox->setValue(1);
    uiTable.timerSpinBox->setValue(4);
    int retval = createTableDialog->exec();
    if (retval == QDialog::Rejected || uiTable.listWidgetTopLevelList->currentItem() == NULL) return;

    // else its not rejected

    writeHeaderData();
    out << (quint8)CLIENT_NEW_TABLE;
    out << uiTable.listWidgetTopLevelList->currentItem()->text();
    out << (quint8)uiTable.playersSpinBox->value();
    out << (quint8)uiMainWindow.comboBoxLexicon->currentIndex();

    if (uiTable.cycleRbo->isChecked()) out << (quint8)TABLE_TYPE_CYCLE_MODE;
    else if (uiTable.endlessRbo->isChecked()) out << (quint8)TABLE_TYPE_MARATHON_MODE;
    else if (uiTable.randomRbo->isChecked()) out << (quint8)TABLE_TYPE_RANDOM_MODE;

    out << (quint8)uiTable.timerSpinBox->value();
    fixHeaderLength();
    commsSocket->write(block);


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
            QString username;
            in >> username;
            gameBoardWidget->setReadyIndicator(username);
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

    case SERVER_TABLE_ALPHAGRAMS:
        // alphagrams!!!
        {
            QTime t;
            t.start();
            quint8 numRacks;
            in >> numRacks;
            for (int i = 0; i < numRacks; i++)
            {
                QString alphagram;
                in >> alphagram;
                quint8 numSolutionsNotYetSolved;
                in >> numSolutionsNotYetSolved;
                QStringList solutions;
                in >> solutions;
                gameBoardWidget->addNewWord(i, alphagram, solutions, numSolutionsNotYetSolved);
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
    case SERVER_TABLE_GUESS_RIGHT:
        {
            QString username, answer;
            quint8 index;
            in >> username >> answer >> index;
            // will change row column to a single number, so hardcode this

            gameBoardWidget->answeredCorrectly(index, username, answer);
            gameBoardWidget->addToPlayerList(username, answer);
            if (username==currentUsername)
                QSound::play("sounds/correct.wav");

        }
        break;
    }


}

void MainWindow::handleWordlistsMessage()
{

    quint8 numLexica;
    in >> numLexica;
    disconnect(uiMainWindow.comboBoxLexicon, SIGNAL(currentIndexChanged(int)), 0, 0);
    uiMainWindow.comboBoxLexicon->clear();
    lexiconLists.clear();
    qDebug() << "Got" << numLexica << "lexica.";
    for (int i = 0; i < numLexica; i++)
    {
        QByteArray lexicon;
        in >> lexicon;
        uiMainWindow.comboBoxLexicon->addItem(lexicon);
        LexiconLists dummyLists;
        dummyLists.lexicon = lexicon;
        lexiconLists << dummyLists;
        qDebug() << dummyLists.lexicon;
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
                quint16 numLists;
                in >> numLists;

                qDebug() << numLists << "regular lists.";
                for (int i = 0; i < numLists; i++)
                {
                    quint8 lexiconIndex;
                    QByteArray listTitle;
                    in >> lexiconIndex >> listTitle;
                    lexiconLists[lexiconIndex].regularWordLists << listTitle;

                }
            }
            break;


        case 'D':
            {
                quint16 numLists;
                in >> numLists;
                qDebug() << numLists << "daily lists.";

                for (int i = 0; i < numLists; i++)
                {
                    quint8 lexiconIndex;
                    QByteArray listTitle;
                    in >> lexiconIndex >> listTitle;
                    lexiconLists[lexiconIndex].dailyWordLists << listTitle;
                }
            }
            break;

        }
    }
    uiMainWindow.comboBoxLexicon->setCurrentIndex(0);
    lexiconComboBoxIndexChanged(0);
    connect(uiMainWindow.comboBoxLexicon, SIGNAL(currentIndexChanged(int)),
            SLOT(lexiconComboBoxIndexChanged(int)));

}

void MainWindow::lexiconComboBoxIndexChanged(int index)
{
    qDebug() << "Changed lexicon to" << index;

    qDebug() << lexiconLists.at(index).regularWordLists.size();
    qDebug() << lexiconLists.at(index).dailyWordLists.size();
    uiTable.listWidgetTopLevelList->clear();
    challengesMenu->clear();
    uiScores.comboBoxChallenges->clear();

    for (int i = 0; i < lexiconLists.at(index).regularWordLists.size(); i++)
    {
        uiTable.listWidgetTopLevelList->addItem(lexiconLists.at(index).regularWordLists.at(i));
    }
    for (int i = 0; i < lexiconLists.at(index).dailyWordLists.size(); i++)
    {
        challengesMenu->addAction(lexiconLists.at(index).dailyWordLists.at(i));
    }
    challengesMenu->addAction("Get today's scores");
    gameBoardWidget->setDatabase(lexiconLists.at(index).lexicon);
    currentLexicon = lexiconLists.at(index).lexicon;
    uiTable.labelLexiconName->setText(currentLexicon);

}


void MainWindow::handleCreateTable(quint16 tablenum, QString wordListDescriptor, quint8 maxPlayers)
{
    tableRepresenter* t = new tableRepresenter;
    t->tableNumItem = new QTableWidgetItem(QString("%1").arg(tablenum));
    t->descriptorItem = new QTableWidgetItem(wordListDescriptor);
    t->maxPlayersItem = new QTableWidgetItem(QString("%1").arg(maxPlayers));
    t->playersItem = new QTableWidgetItem("");
    t->numPlayersItem = new QTableWidgetItem("0");
    t->buttonItem = new QPushButton("Join");
    t->buttonItem->setProperty("tablenum", QVariant((quint16)tablenum));
    t->tableNum = tablenum;
    connect(t->buttonItem, SIGNAL(clicked()), this, SLOT(joinTable()));
    t->buttonItem->setEnabled(false);

    //  QStringList playerList;
    //t->playersItem->setData(PLAYERLIST_ROLE, QVariant(playerList));

    int rc = uiMainWindow.roomTableWidget->rowCount();

    uiMainWindow.roomTableWidget->insertRow(rc);
    uiMainWindow.roomTableWidget->setItem(rc, 0, t->tableNumItem);
    uiMainWindow.roomTableWidget->setItem(rc, 1, t->descriptorItem);
    uiMainWindow.roomTableWidget->setItem(rc, 2, t->playersItem);
    uiMainWindow.roomTableWidget->setItem(rc, 3, t->numPlayersItem);
    uiMainWindow.roomTableWidget->setItem(rc, 4, t->maxPlayersItem);
    uiMainWindow.roomTableWidget->setCellWidget(rc, 5, t->buttonItem);

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
        textToSet += thisplayer + " ";

    int numPlayers = t->playerList.size();
    t->playersItem->setText(textToSet);
    t->numPlayersItem->setText(QString("%1").arg(numPlayers));

    quint8 maxNumPlayers = t->maxPlayersItem->text().toShort();

    if (numPlayers >= maxNumPlayers)
        t->buttonItem->setEnabled(false);
    else
        t->buttonItem->setEnabled(true);
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
        textToSet += thisplayer + " ";

    t->playersItem->setText(textToSet);

    quint8 numPlayers = t->playerList.size();
    quint8 maxPlayers = t->maxPlayersItem->text().toShort();
    t->numPlayersItem->setText(QString("%1").arg(numPlayers));

    if (numPlayers >= maxPlayers)
    {
        t->buttonItem->setEnabled(false);
    }
    else
    {
        t->buttonItem->setEnabled(true);
    }
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
    QString infoText;
    infoText += "Special thanks to the following people who have helped me in the development of Aerolith in some way, and to anyone else I may have forgotten:";
    infoText += "<BR>Joseph Bihlmeyer, Brian Bowman, Doug Brockmeier, Benjamin Dweck, Monique Kornell, Danny McMullan, David Wiegand, Gabriel Wong, Joanna Zhou, Aaron Bader, Maria Ho, Elana Lehrer, James Leong, Seth Lipkin, Kenji Matsumoto, Michael Thelen.<BR>";
    QMessageBox::information(this, "Acknowledgements", infoText);
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

void MainWindow::changeMyAvatar(quint8 avatarID)
{

    writeHeaderData();
    out << (quint8) CLIENT_AVATAR << avatarID;
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
        out << challengeAction->text(); // create a table
        out << (quint8)1; // 1 player
        out << (quint8)TABLE_TYPE_DAILY_CHALLENGES; //  (TODO: HARDCODE BAD)
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
    uiLogin.connectStatusLabel->setText("Server thread has started! Log in now.");
    uiLogin.pushButtonStartOwnServer->setText("Stop Server");
    uiLogin.serverLE->setText("localhost");
    uiLogin.portLE->setText("1988");
}

void MainWindow::serverThreadHasFinished()
{

    uiLogin.connectStatusLabel->setText("Server thread has stopped!");
    uiLogin.pushButtonStartOwnServer->setText("Start Own Server");

    uiLogin.serverLE->setText("aerolith.org");
    uiLogin.portLE->setText("1988");
    uiLogin.loginPushButton->setEnabled(true);

}

/////////////////////////////////////////////////////

PMWidget::PMWidget(QWidget* parent, QString senderUsername, QString receiverUsername) : 
        QWidget(parent), senderUsername(senderUsername), receiverUsername(receiverUsername)
{

    uiPm.setupUi(this);
    connect(uiPm.lineEdit, SIGNAL(returnPressed()), this, SLOT(readAndSendLEContents()));
    setWindowTitle("Conversation with " + receiverUsername);
}

void PMWidget::readAndSendLEContents()
{
    emit sendPM(receiverUsername, uiPm.lineEdit->text());
    uiPm.textEdit->append("[" + senderUsername + "] " + uiPm.lineEdit->text());

    uiPm.lineEdit->clear();
    QSound::play("sounds/outbound.wav");
}

void PMWidget::appendText(QString text)
{
    uiPm.textEdit->append(text);
}
