#define UNICODE
#define QT_NO_DEBUG
#define QT_CORE_LIB
#define QT_GUI_LIB
#define QT_THREAD_SUPPORT

#include <QApplication>
#include <QtGui>
//Q_IMPORT_PLUGIN(QJpegPlugin)
//Q_IMPORT_PLUGIN(QMngPlugin)

#include <../base.hpp>
#include <../cart/cart.hpp>

#include <nall/config.hpp>
#include <nall/function.hpp>
#include <nall/input.hpp>
using namespace nall;

#include <ruby/ruby.hpp>
using namespace ruby;

#include <libfilter/libfilter.hpp>

#include "input/input.hpp"
#include "utility/utility.hpp"

class Application {
public:
  class App : public QApplication {
  public:
    #ifdef _WIN32
    bool winEventFilter(MSG *msg, long *result);
    #endif
    App(int argc, char **argv) : QApplication(argc, argv) {}
  } *app;

  bool terminate;  //set to true to terminate main() loop and exit emulator
  bool power;
  bool pause;
  bool autopause;

  string configFilename;
  string styleSheetFilename;

  int main(int argc, char **argv);
  void processEvents();
  void locateFile(string &filename, bool createDataDirectory = false);
  void initPaths(const char *basename);
  void init();

  Application();
  ~Application();
} application;

struct Style {
  enum {
    WindowMargin     = 5,
    WidgetSpacing    = 5,
    SeparatorSpacing = 5,
  };
};
