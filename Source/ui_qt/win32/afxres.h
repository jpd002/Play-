// Res.rc needs to include afxres.h which is only found in specific Visual studio editions
// this is used as a work around as winres.h can be used as a replacement
// but Res.rc would automatically be overwritten, while this wont.
#include "winres.h"
