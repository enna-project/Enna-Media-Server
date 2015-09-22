#include <QCoreApplication>
#include <QSettings>

#include "Application.h"

#include <QtGlobal>
#include <QTime>
#include <iostream>

#define COLOR_LIGHTRED  "\033[31;1m"
#define COLOR_RED       "\033[31m"
#define COLOR_LIGHTBLUE "\033[34;1m"
#define COLOR_BLUE      "\033[34m"
#define COLOR_GREEN     "\033[32;1m"
#define COLOR_YELLOW    "\033[33;1m"
#define COLOR_ORANGE    "\033[0;33m"
#define COLOR_WHITE     "\033[37;1m"
#define COLOR_LIGHTCYAN "\033[36;1m"
#define COLOR_CYAN      "\033[36m"
#define COLOR_RESET     "\033[0m"
#define COLOR_HIGH      "\033[1m"


#define LOG_WRITE(OUTPUT, COLOR, LEVEL, MSG) OUTPUT << COLOR << \
QTime::currentTime().toString("hh:mm:ss:zzz").toStdString() << \
" " LEVEL " " << COLOR_RESET << MSG << "\n"

static void EmsCustomLogWriter(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{

   switch (type)
     {
      case QtWarningMsg:
         LOG_WRITE(std::cout, COLOR_YELLOW, "WARN", msg.toStdString());
         break;
      case QtCriticalMsg:
         LOG_WRITE(std::cout, COLOR_RED, "CRIT",  msg.toStdString());
         break;
      case QtFatalMsg:
         LOG_WRITE(std::cout, COLOR_ORANGE, "FATAL",  msg.toStdString());
         break;
      case QtDebugMsg:
         LOG_WRITE(std::cout, COLOR_CYAN, "DEBUG",  msg.toStdString());
         break;
      default:
         LOG_WRITE(std::cout, COLOR_CYAN, "DEBUG",  msg.toStdString());
         break;
     }
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(EmsCustomLogWriter);
    Application a(argc, argv);
    
    return a.exec();
}
