#pragma once

#include "common.h"

extern "C" {

MsgList *init_be(int debug);

void clear_msg();

void deinit_be();

}