#include "serverthread.h"
#include <QDataStream>
ServerThread::ServerThread(int socketDescriptor, QObject *parent) : QThread(parent), socketDescriptor(socketDescriptor)
{
 
}

void ServerThread::run()
{
  QTcpSocket tcpSocket;

  if (!tcpSocket.setSocketDescriptor(socketDescriptor))
    {
      emit error(tcpSocket.error());
      return;
    }

  QByteArray block;
  QDataStream out(&block, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_4_0);
  out << (quint16)0;
  out << QString("Welcome to Aerolith.");
  out.device()->seek(0);
  out << (quint16)(block.size()- sizeof(quint16));

  tcpSocket.write(block);
  tcpSocket.disconnectFromHost();
  tcpSocket.waitForDisconnected();

}
