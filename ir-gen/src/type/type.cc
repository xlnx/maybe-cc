#include "type.h"

static TypeView &getCharTy( bool is_signed );
static TypeView &getShortTy( bool is_signed );
static TypeView &getIntTy( bool is_signed );
static TypeView &getLongTy( bool is_signed );
static TypeView &getLongLongTy( bool is_signed );
static TypeView &getFloatTy();
static TypeView &getDoubleTy();
static TypeView &getLongDoubleTy();