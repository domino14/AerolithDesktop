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


#include "listmaker.h"
#include <QTime>

extern const QString WORD_DATABASE_NAME = "wordDB";


extern const QString WORD_DATABASE_FILENAME = "words.db";

/*http://bytes.com/forum/thread138180.html for discussion on having to declare extern explicitly
with const */

QStringList ListMaker::lexiconList;
void ListMaker::testDatabaseTime()
{
    QTime timer;
    QString strings[10000];
    QSqlQuery wordQuery(QSqlDatabase::database(WORD_DATABASE_NAME));
    timer.start();
    int strIndex;
    wordQuery.exec("SELECT alphagram, words from alphagrams where length = 8 and probability between 5000 and 6000");
    qDebug() << "1: " << timer.elapsed();
    strIndex = 0;
    while (wordQuery.next())
    {
        strings[strIndex] = wordQuery.value(0).toString() + "---" + wordQuery.value(1).toString();
        strIndex++;
    }

    qDebug() << "2: " << timer.elapsed();

    for (int i = 0; i < strIndex; i++)
        qDebug() << i << strings[i];

}

void ListMaker::createListDatabase()
{
    lexiconList.clear();
    // creates a word list database.
    // requires the zyzzyva database to be in the user's install directory
    lexiconList << "OWL2+LWL" << "CSW" << "Volost";

    QSqlDatabase wordDb;
    wordDb = QSqlDatabase::addDatabase("QSQLITE", WORD_DATABASE_NAME);

    /* note. we must copy the words.db to the executable path inside the bundle for Mac OS in order
       for the following to work */
    QString dbFilename =  QCoreApplication::applicationDirPath() + "/" + WORD_DATABASE_FILENAME;
    wordDb.setDatabaseName(dbFilename);
    wordDb.open();

    QSqlQuery wordQuery(QSqlDatabase::database(WORD_DATABASE_NAME));
    bool alphagramsExists = false;
    wordQuery.exec("select tbl_name from sqlite_master where name='alphagrams'");
    while (wordQuery.next())
    {
        if (wordQuery.value(0).toString() == "alphagrams")
            alphagramsExists = true;
    }
    qDebug() << "alphagramsExists" << alphagramsExists;

    if (!alphagramsExists)
    {
        QSqlQuery wordQuery(QSqlDatabase::database(WORD_DATABASE_NAME));
        wordQuery.exec("CREATE TABLE IF NOT EXISTS lexica(name VARCHAR(15), index INTEGER");
        for (int i = 0; i < lexiconList.size(); i++)
            wordQuery.exec(QString("INSERT INTO lexica(name, index) VALUES('%1', %2)").arg(lexiconList.at(i)).arg(i));

        wordQuery.exec("CREATE TABLE IF NOT EXISTS alphagrams(alphagram VARCHAR(15), words VARCHAR(255), "
                       "length INTEGER, probability INTEGER, num_vowels INTEGER, lexiconstrings VARCHAR(50), "
                       "num_anagrams INTEGER, num_unique_letters INTEGER, lexiconName VARCHAR(10), "
                       "definitions VARCHAR(2560), front_hooks VARCHAR(256), back_hooks VARCHAR(256))");

        wordQuery.exec("CREATE TABLE IF NOT EXISTS wordlists(listname VARCHAR(40), wordlength INTEGER, numalphagrams INTEGER, "
                       "probindices BLOB, lexiconName VARCHAR(10))");

        for (int i = 0; i < lexiconList.size(); i++)
        {
            createLexiconDatabase(i);

        }
        wordQuery.exec("CREATE INDEX listname_index on wordlists(listname, lexiconName)");
        wordQuery.exec("CREATE UNIQUE INDEX probability_index on alphagrams(probability, length, lexiconName)");
        wordQuery.exec("CREATE UNIQUE INDEX alphagram_index on alphagrams(alphagram, lexiconName)");
    }
    else
        qDebug() << "Don't need to create, just connect.";
    //QSqlDatabase::removeDatabase(WORD_DATABASE_NAME);
}

void ListMaker::createLexiconDatabase(int lexiconIndex)
{
    QString lexiconName = lexiconList[lexiconIndex];
    QSqlDatabase zyzzyvaDb;

#ifdef Q_WS_MAC
    QSettings ZyzzyvaSettings("pietdepsi.com", "Zyzzyva");
#else
    QSettings ZyzzyvaSettings("Piet Depsi", "Zyzzyva");
#endif
    ZyzzyvaSettings.beginGroup("Zyzzyva");

    QString defaultUserDir = QDir::homePath() + "/.zyzzyva";
    QString zyzzyvaDataDir = QDir::cleanPath (ZyzzyvaSettings.value("user_data_dir", defaultUserDir).toString());

    QString dbPath = zyzzyvaDataDir + "/lexicons/" + lexiconName + ".db";

    if (QFile::exists(dbPath))
    {
        zyzzyvaDb = QSqlDatabase::addDatabase("QSQLITE", "zyzzyvaDB");
        zyzzyvaDb.setDatabaseName(dbPath);
        zyzzyvaDb.open();
    }
    else
    {
        qDebug() << "That database was not found. Cannot generate word lists for" << lexiconName << "!";
        return;
    }

    QSqlQuery wordQuery(QSqlDatabase::database(WORD_DATABASE_NAME));
    QSqlQuery zyzzyvaQuery(QSqlDatabase::database("zyzzyvaDB"));
    QTime time;
    time.start();
    for (int i = 2; i <= 15; i++)
    {
        qDebug() << lexiconName << i;
        QString queryString = QString("SELECT word, alphagram, lexicon_symbols, num_vowels, num_anagrams, num_unique_letters, "
                                      "definition, front_hooks, back_hooks "
                                      "from words where length = %1 order by probability_order").arg(i);
        zyzzyvaQuery.exec(queryString);
        int probability = 1;
        bool nextSucceeded = zyzzyvaQuery.next();
        wordQuery.exec("BEGIN TRANSACTION");
        while (nextSucceeded)
        {

            int num_anagrams = zyzzyvaQuery.value(4).toInt();
            QString alphagram;
            QString wordString = "";
            QString lexiconString = "";
            QString definitions, front_hooks, back_hooks;
            int numVowels;
            int numAnagrams;
            int numUniqueLetters;

            for (int n = 0; n < num_anagrams; n++)
            {
                alphagram = zyzzyvaQuery.value(1).toString();
                wordString += zyzzyvaQuery.value(0).toString() + " ";
                lexiconString += zyzzyvaQuery.value(2).toString() + "@";
                numVowels = zyzzyvaQuery.value(3).toInt();
                numAnagrams = zyzzyvaQuery.value(4).toInt();
                numUniqueLetters = zyzzyvaQuery.value(5).toInt();
                definitions += zyzzyvaQuery.value(6).toString() + "@";
                front_hooks += zyzzyvaQuery.value(7).toString() + "@";
                back_hooks += zyzzyvaQuery.value(8).toString() + "@";
                nextSucceeded = zyzzyvaQuery.next();

            }
            int wordlength = alphagram.length();
            QString toExecute = "INSERT INTO alphagrams(alphagram, words, length, probability, num_vowels, lexiconstrings, "
                                "num_anagrams, num_unique_letters, lexiconName, definitions, front_hooks, back_hooks) "
                                "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
            wordQuery.prepare(toExecute);
            wordQuery.bindValue(0, alphagram);
            wordQuery.bindValue(1, wordString.trimmed());
            wordQuery.bindValue(2, wordlength);
            wordQuery.bindValue(3, probability + (wordlength << 24));
            wordQuery.bindValue(4, numVowels);
            wordQuery.bindValue(5, lexiconString);
            wordQuery.bindValue(6, numAnagrams);
            wordQuery.bindValue(7, numUniqueLetters);
            wordQuery.bindValue(8, lexiconName);
            wordQuery.bindValue(9, definitions);
            wordQuery.bindValue(10, front_hooks);
            wordQuery.bindValue(11, back_hooks);
            probability++;

            wordQuery.exec();

            // if (!nextSucceeded) break;
        }
        wordQuery.exec("END TRANSACTION");
    }

    qDebug() << "Created alphas in" << time.elapsed() << "for lexicon" << lexiconName;





    QVector <int> pick;
    /* top 500 by prob 6s - 9s */
    pick.append(7);
    pick.append(8);
    pick.append(6);
    pick.append(9);
    wordQuery.exec("BEGIN TRANSACTION");
    foreach (int i, pick)
    {
        int probIndex = 1;

        int modifiedProbIndex = 1 + (i << 24); // i << 24 is basically encoding the length of the alphagram in this number
                                        // this makes it easier to have different anagram lengths for a table.
        int numAlphagrams = 0;
        wordQuery.exec(QString("SELECT count(*) from alphagrams where length = %1 and lexiconName = '%2'").
                       arg(i).arg(lexiconName));

        while (wordQuery.next())
        {
            numAlphagrams = wordQuery.value(0).toInt();
        }
        qDebug() << "Count:" << i << numAlphagrams;

        int listLength = 500;
        int actualListLength;
        do
        {
            QByteArray ba;
            QDataStream baWriter(&ba, QIODevice::WriteOnly);

            wordQuery.exec(QString("SELECT count(*) from alphagrams where length = %1 and probability between %2 and %3"
                                   " and lexiconName = '%4'").
                           arg(i).arg(modifiedProbIndex).arg(modifiedProbIndex + listLength -1).arg(lexiconName));

            while (wordQuery.next())
                actualListLength = wordQuery.value(0).toInt();
        //    qDebug() << " .." << actualListLength;

            if (actualListLength == 0) continue;
            baWriter << (quint8)0 << (quint8)i << (quint32)(modifiedProbIndex) << (quint32)(modifiedProbIndex+actualListLength-1);
            // (quint8)0 means this is a RANGE of indices, and not a list of indices
            // second param is word length.


            QString toExecute = "INSERT INTO wordlists(listname, wordlength, numalphagrams, probindices, lexiconName) "
                                "VALUES(?, ?, ?, ?, ?)";
            wordQuery.prepare(toExecute);
            wordQuery.bindValue(0, QString("Useful %1s (%2 - %3)").arg(i).arg(probIndex).arg(probIndex + actualListLength-1));
            wordQuery.bindValue(1, i);
            wordQuery.bindValue(2, actualListLength);
            wordQuery.bindValue(3, ba);
            wordQuery.bindValue(4, lexiconName);
            wordQuery.exec();

            probIndex+= 500;
            modifiedProbIndex += 500;

        } while (probIndex < numAlphagrams);
    }

    for (int i = 2; i <= 15; i++)
    {
        int numAlphagrams;
        QByteArray ba;
        QDataStream baWriter(&ba, QIODevice::WriteOnly);
        wordQuery.exec(QString("SELECT count(*) from alphagrams where length = %1 and lexiconName = '%2'").arg(i).arg(lexiconName));
        while (wordQuery.next())
        {
            numAlphagrams = wordQuery.value(0).toInt();
        }
        if (numAlphagrams == 0) continue;
        baWriter << (quint8)0 << (quint8)i << (quint32)(1 + (i << 24)) << (quint32)(numAlphagrams + (i << 24));
        QString toExecute = "INSERT INTO wordlists(listname, wordlength, numalphagrams, probindices, lexiconName) "
                            "VALUES(?, ?, ?, ?, ?)";
        wordQuery.prepare(toExecute);
        wordQuery.bindValue(0, QString(lexiconName+ " %1s").arg(i));
        wordQuery.bindValue(1, i);
        wordQuery.bindValue(2, numAlphagrams);
        wordQuery.bindValue(3, ba);
        wordQuery.bindValue(4, lexiconName);
        wordQuery.exec();
    }

    QString vowelQueryString = "SELECT probability from alphagrams where length = %1 and num_vowels = %2 and lexiconName = '%3'";
    sqlListMaker(vowelQueryString.arg(8).arg(5).arg(lexiconName), "Five-vowel-8s", 8, lexiconName);
    sqlListMaker(vowelQueryString.arg(7).arg(4).arg(lexiconName), "Four-vowel-7s", 7, lexiconName);
    QString jqxzQueryString = "SELECT probability from alphagrams where length = %1 and lexiconName = '%2' and "
                              "(alphagram like '%Q%' or alphagram like '%J%' or alphagram like '%X%' or alphagram like '%Z%')";

    for (int i = 4; i <= 8; i++)
        sqlListMaker(jqxzQueryString.arg(i).arg(lexiconName), QString("JQXZ %1s").arg(i), i, lexiconName);

    if (lexiconName == "OWL2+LWL")
    {
        QString newWordsQueryString = "SELECT probability from alphagrams where length = %1 and lexiconName = '%2' and "
                                      "lexiconstrings like '%*%%' ESCAPE '*'";
        for (int i = 7; i <= 8; i++)
            sqlListMaker(newWordsQueryString.arg(i).arg(lexiconName), QString("New (OWL2) %1s").arg(i), i, lexiconName);
    }
    else if (lexiconName == "CSW")
    {
        QString newWordsQueryString = "SELECT probability from alphagrams where length = %1 and lexiconName = '%2' and "
                                      "lexiconstrings like '%#%'";
        for (int i = 7; i <= 8; i++)
            sqlListMaker(newWordsQueryString.arg(i).arg(lexiconName), QString("CSW-only %1s").arg(i), i, lexiconName);
    }

    QString singleAnagramsQueryString = "SELECT probability from alphagrams where length = %1 and num_anagrams = 1 and lexiconName = '%2'";
    QString moreThanOneQueryString = "SELECT probability from alphagrams where length = %1 and num_anagrams > 1 and lexiconName = '%2'";

    for (int i = 7; i <= 8; i++)
    {
        sqlListMaker(singleAnagramsQueryString.arg(i).arg(lexiconName), QString("One-anagram %1s").arg(i), i, lexiconName);
        sqlListMaker(moreThanOneQueryString.arg(i).arg(lexiconName), QString("Multi-anagram %1s").arg(i), i, lexiconName);
    }

    QString uniqueLettersQueryString = "SELECT probability from alphagrams where num_unique_letters = %1 and length = %2 and lexiconName = '%3'";
    for (int i = 7; i <= 8; i++)
        sqlListMaker(uniqueLettersQueryString.arg(i).arg(i).arg(lexiconName), QString("Unique-letter %1s").arg(i), i, lexiconName);

    wordQuery.exec("END TRANSACTION");
    zyzzyvaDb.close();
    QSqlDatabase::removeDatabase("zyzzyvaDB");
}

void ListMaker::sqlListMaker(QString queryString, QString listName, quint8 wordLength, QString lexiconName)
{
    QSqlQuery wordQuery(QSqlDatabase::database(WORD_DATABASE_NAME));
    wordQuery.exec(queryString);
    QVector <quint32> probIndices;

    while (wordQuery.next())
    {
        probIndices.append(wordQuery.value(0).toInt());
    }
    qDebug() << listName << "found" << probIndices.size();
    if (probIndices.size() == 0) return;

    QByteArray ba;
    QDataStream baWriter(&ba, QIODevice::WriteOnly);

    baWriter << (quint8)1 << (quint8)wordLength << (quint16)probIndices.size();

    // (quint8)1 means this is a LIST of indices
    // second param is word length.
    // third param is number of indices
    foreach(quint32 index, probIndices)
        baWriter << index;

    QString toExecute = "INSERT INTO wordlists(listname, wordlength, numalphagrams, probindices, lexiconName) "
                        "VALUES(?,?,?,?,?)";
    wordQuery.prepare(toExecute);
    wordQuery.bindValue(0, listName);
    wordQuery.bindValue(1, wordLength);
    wordQuery.bindValue(2, probIndices.size());
    wordQuery.bindValue(3, ba);
    wordQuery.bindValue(4, lexiconName);
    wordQuery.exec();


}

