//
// wsjtx_app_version - a console application that outputs WSJT-X
//                     application version
//
// This application is only provided as a simple console application
//
//

#include <cstdlib>
#include <iostream>
#include <exception>

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>

#include "revision_utils.hpp"

int main (int argc, char * argv[])
{
  QCoreApplication app {argc, argv};
  try
    {
      app.setApplicationName ("NBMODES-IC8TEM"); // IC8TEM: own name, own settings folder, UDP id
      app.setApplicationVersion (QStringLiteral (NBM_VERSION_STR)); // IC8TEM: NBMODES-IC8TEM version

      QCommandLineParser parser;
//      parser.setApplicationDescription ("\n" PROJECT_DESCRIPTION);
      parser.addHelpOption ();
      parser.addVersionOption ();
      parser.process (app);
      return EXIT_SUCCESS;
    }
  catch (std::exception const& e)
    {
      std::cerr << "Error: " << e.what () << '\n';
    }
  catch (...)
    {
      std::cerr << "Unexpected error\n";
    }
  return -1;
}
