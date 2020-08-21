#include <X11/Xlib.h>
#include <X11/Xutil.h>

// Nasledujici promenne jsou vyzadujovany nekterymi X-ovymi funkcemi
Display *display;
int screen;

void initX(); // Zahaji komunikaci s X serverem, inicializuje promenne vyse.
